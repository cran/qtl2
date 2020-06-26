// re-estimate inter-marker recombination fractions
// this set of versions is tailored somewhat to cross type, to be faster
// est_map2_lowmem: just use the standard approach
// est_map2_grouped: same transition matrix within groups having common sex & cross_info
//                   (bc-type, f2, riself4)
// est_map2_forder: need to deal with founder order in transition matrix
//                  (riself8, riself16)

#include "hmm_estmap2.h"
#include <math.h>
#include <Rcpp.h>
#include "cross.h"
#include "hmm_util.h"
#include "hmm_forwback2.h"
#include "hmm_estmap.h"
#include "r_message.h" // defines RQTL2_NODEBUG and r_message()
#include "cross_util.h"

// re-estimate inter-marker recombination fractions
//
// this approach pre-calculates init, emit, step; uses more memory but faster
//
// cross_group and unique_cross_group are used in est_map_grouped
//     cross_group = vector of integers that categorizes individuals into groups with common is_female and cross_info
//                   values in {0, 1, ..., length(unique_cross_group)-1}
//     unique_cross_group = vector of indexes to the first individual in each category, for grabbing is_female
//                          and cross_info for the category
//
// [[Rcpp::export(".est_map2")]]
NumericVector est_map2(const String& crosstype,
                       const IntegerMatrix& genotypes, // columns are individuals, rows are markers
                       const IntegerMatrix& founder_geno, // columns are markers, rows are founder lines
                       const bool is_X_chr,
                       const LogicalVector& is_female,
                       const IntegerMatrix& cross_info,
                       const IntegerVector& cross_group, // categories of unique (is_fem, cross_inf)
                       const IntegerVector& unique_cross_group, // index to the unique ones
                       const NumericVector& rec_frac,
                       const double error_prob,
                       const int max_iterations,
                       const double tol,
                       const bool verbose)
{
    const int n_ind = genotypes.cols();
    const int n_mar = genotypes.rows();
    const int n_rf = n_mar-1;

    QTLCross* cross = QTLCross::Create(crosstype);

    // check inputs
    if(is_female.size() != n_ind)
        throw std::range_error("length(is_female) != ncol(genotypes)");
    if(cross_info.cols() != n_ind)
        throw std::range_error("ncols(cross_info) != ncol(genotypes)");
    if(rec_frac.size() != n_rf)
        throw std::range_error("length(rec_frac) != nrow(genotypes)-1");

    if(error_prob < 0.0 || error_prob > 1.0)
        throw std::range_error("error_prob out of range");

    for(int i=0; i<rec_frac.size(); i++) {
        if(rec_frac[i] < 0 || rec_frac[i] > 0.5)
            throw std::range_error("rec_frac must be >= 0 and <= 0.5");
    }

    if(max_iterations < 0)
      throw std::range_error("max_iterations should be >= 0");
    if(tol < 0)
      throw std::range_error("tol >= 0");

    if(!cross->check_founder_geno_size(founder_geno, n_mar))
        throw std::range_error("founder_geno is not the right size");
    // end of checks

    NumericVector result = cross->est_map2(genotypes, founder_geno,
                                           is_X_chr, is_female, cross_info,
                                           cross_group, unique_cross_group,
                                           rec_frac, error_prob, max_iterations,
                                           tol, verbose);

    delete cross;
    return result;
}


// just use the low-mem approach
NumericVector est_map2_lowmem(const String crosstype,
                              const IntegerMatrix& genotypes, // columns are individuals, rows are markers
                              const IntegerMatrix& founder_geno, // columns are markers, rows are founder lines
                              const bool is_X_chr,
                              const LogicalVector& is_female,
                              const IntegerMatrix& cross_info,
                              const IntegerVector& cross_group, // categories of unique (is_fem, cross_inf)
                              const IntegerVector& unique_cross_group, // index to the unique ones
                              const NumericVector& rec_frac,
                              const double error_prob,
                              const int max_iterations,
                              const double tol,
                              const bool verbose)
{
    return est_map(crosstype, genotypes, founder_geno,
                   is_X_chr, is_female, cross_info,
                   rec_frac, error_prob, max_iterations,
                   tol, verbose);
}


// same init, emit, step for groups with common sex and cross_info
//
//     cross_group = vector of integers that categorizes individuals into groups with common is_female and cross_info
//                   values in {0, 1, ..., length(unique_cross_group)-1}
//     unique_cross_group = vector of indexes to the first individual in each category, for grabbing is_female
//                          and cross_info for the category
//
NumericVector est_map2_grouped(const String crosstype,
                               const IntegerMatrix& genotypes, // columns are individuals, rows are markers
                               const IntegerMatrix& founder_geno, // columns are markers, rows are founder lines
                               const bool is_X_chr,
                               const LogicalVector& is_female,
                               const IntegerMatrix& cross_info,
                               const IntegerVector& cross_group, // categories of unique (is_fem, cross_inf)
                               const IntegerVector& unique_cross_group, // index to the unique ones
                               const NumericVector& rec_frac,
                               const double error_prob,
                               const int max_iterations,
                               const double tol,
                               const bool verbose)
{
    const int n_ind = genotypes.cols();
    const int n_mar = genotypes.rows();
    const int n_rf = n_mar-1;
    const int n_cross_group = unique_cross_group.size();

    const double rf_tol = tol/1000.0; // smallest allowed recombination fraction
    const double rf_uptol = 0.999;    // largest allowed recombination fraction

    #ifndef RQTL2_NODEBUG
    for(int i=0; i<n_cross_group; i++) {
        if(unique_cross_group[i] < 0 || unique_cross_group[i] >= n_ind)
            throw std::range_error("unique_cross_group values out of range [0, n_ind-1]");
    for(int i=0; i<n_ind; i++) {
        if(cross_group[i] < 0 || cross_group[i] >= n_cross_group)
            throw std::range_error("cross_group values out of range [0, n_group-1]");
    #endif

    QTLCross* cross_pu = QTLCross::Create(crosstype);
    QTLCross* cross;
    if(cross_pu->crosstype != cross_pu->phase_known_crosstype) // get phase-known version of cross
        cross = QTLCross::Create(cross_pu->phase_known_crosstype);
    else cross = cross_pu;

    NumericVector cur_rec_frac(n_rf);
    NumericVector prev_rec_frac(clone(rec_frac));

    // marker index for forward/backward equations
    IntegerVector marker_index(n_mar);
    for(int i=0; i<n_mar; i++) marker_index[i] = i;

    // 3-d array to contain sum(gamma(il,ir)) for each interval
    const int n_gen = cross->ngen(is_X_chr);
    const int n_gen_sq = n_gen*n_gen;
    const int n_gen_sq_times_n_ind = n_gen_sq * n_ind;
    NumericVector full_gamma(n_gen_sq_times_n_ind * n_rf);

    // pre-calculate stuff; need separate ones for each unique value of is_female/cross_info
    const int max_obsgeno = max(genotypes);
    std::vector<std::vector<NumericMatrix> > emit_matrix(n_cross_group);
    std::vector<NumericVector> init_vector(n_cross_group);
    std::vector<IntegerVector> poss_gen(n_cross_group);
    IntegerVector n_poss_gen(n_cross_group);

    for(int i=0; i<n_cross_group; i++) {
        emit_matrix[i] = cross->calc_emitmatrix(error_prob, max_obsgeno,
                                                founder_geno, is_X_chr,
                                                is_female[unique_cross_group[i]],
                                                cross_info(_,unique_cross_group[i]));
        init_vector[i] = cross->calc_initvector(is_X_chr,
                                                is_female[unique_cross_group[i]],
                                                cross_info(_,unique_cross_group[i]));
        poss_gen[i] = cross->possible_gen(is_X_chr,
                                          is_female[unique_cross_group[i]],
                                          cross_info(_,unique_cross_group[i]));
        n_poss_gen[i] = poss_gen[i].size();
    }

    bool converged = false; // flag for convergence
    for(int it=0; it<max_iterations; it++) {

        // transition matrix for current rec fracs
        std::vector<std::vector<NumericMatrix> > step_matrix(n_cross_group);
        for(int i=0; i<n_cross_group; i++) {
            step_matrix[i] = cross->calc_stepmatrix(prev_rec_frac, is_X_chr,
                                                    is_female[unique_cross_group[i]],
                                                    cross_info(_,unique_cross_group[i]));
        }

        // zero the full_gamma array
        full_gamma.fill(0.0);

        for(int ind=0; ind < n_ind; ind++) {
            Rcpp::checkUserInterrupt();  // check for ^C from user

            const int this_n_poss_gen = n_poss_gen[cross_group[ind]];

            // forward and backward equations
            NumericMatrix alpha = forwardEquations2(genotypes(_,ind),
                                                    init_vector[cross_group[ind]],
                                                    emit_matrix[cross_group[ind]],
                                                    step_matrix[cross_group[ind]],
                                                    marker_index,
                                                    poss_gen[cross_group[ind]]);
            NumericMatrix beta = backwardEquations2(genotypes(_,ind),
                                                    init_vector[cross_group[ind]],
                                                    emit_matrix[cross_group[ind]],
                                                    step_matrix[cross_group[ind]],
                                                    marker_index,
                                                    poss_gen[cross_group[ind]]);

            for(int pos=0; pos<n_rf; pos++) {
                // calculate gamma = log Pr(v1, v2, O)
                NumericMatrix gamma(this_n_poss_gen, this_n_poss_gen);
                double sum_gamma=0.0;
                bool sum_gamma_undef = true;
                for(int ir=0; ir<this_n_poss_gen; ir++) {
                    for(int il=0; il<this_n_poss_gen; il++) {
                        gamma(il,ir) = alpha(il,pos) + beta(ir,pos+1) +
                            emit_matrix[cross_group[ind]][pos+1](genotypes(pos+1,ind),ir) +
                            step_matrix[cross_group[ind]][pos](il,ir);

                        if(sum_gamma_undef) {
                            sum_gamma_undef = false;
                            sum_gamma = gamma(il,ir);
                        }
                        else {
                            sum_gamma = addlog(sum_gamma, gamma(il,ir));
                        }
                    }
                }

                // add to full_gamma array of dim n_rf x n_ind x n_gen x n_gen
                const int offset = n_gen_sq_times_n_ind*pos + n_gen_sq*ind;
                for(int ir=0; ir<this_n_poss_gen; ir++) {
                    int gr_by_n_gen = (poss_gen[cross_group[ind]][ir]-1)*n_gen;
                    for(int il=0; il<this_n_poss_gen; il++) {
                        int gl = poss_gen[cross_group[ind]][il]-1;
                        full_gamma[offset + gr_by_n_gen + gl] += exp(gamma(il,ir) - sum_gamma);
                    }
                }

            } // loop over marker intervals


        } // loop over individuals

        // re-estimate rec'n fractions
        for(int pos=0; pos < n_rf; pos++) {
            // pull out the part for that position
            NumericVector sub_gamma(n_gen_sq_times_n_ind);
            std::copy(full_gamma.begin()+(n_gen_sq_times_n_ind*pos),
                      full_gamma.begin()+(n_gen_sq_times_n_ind*(pos+1)),
                      sub_gamma.begin());
            cur_rec_frac[pos] = cross->est_rec_frac(sub_gamma, is_X_chr, cross_info, n_gen);
        }

        // don't let rec fracs get too small
        for(int pos=0; pos<n_rf; pos++) {
            if(cur_rec_frac[pos] < rf_tol) cur_rec_frac[pos] = rf_tol;
            if(cur_rec_frac[pos] > rf_uptol) cur_rec_frac[pos] = rf_uptol;
        }

        if(verbose) {
            double maxdif = max(abs(prev_rec_frac - cur_rec_frac));
            Rprintf("%4d %.12f\n", it+1, maxdif);
        }

        // check convergence
        converged = true;
        for(int pos=0; pos<n_rf; pos++) {
            if(fabs(prev_rec_frac[pos] - cur_rec_frac[pos]) > tol*(cur_rec_frac[pos]+tol*100.0)) {
                converged = false;
                break;
            }
        }

        if(converged) break;

        prev_rec_frac = clone(cur_rec_frac);
    } // end loop over iterations

    if(!converged)
        r_warning("est_map reaching maximum iterations without converging");

    // transition matrix for current rec fracs
    std::vector<std::vector<NumericMatrix> > step_matrix(n_cross_group);
    for(int i=0; i<n_cross_group; i++) {
        step_matrix[i] = cross->calc_stepmatrix(cur_rec_frac, is_X_chr,
                                                is_female[unique_cross_group[i]],
                                                cross_info(_,unique_cross_group[i]));
    }


    // calculate log likelihood
    double loglik = 0.0;
    for(int ind=0; ind<n_ind; ind++) {
        Rcpp::checkUserInterrupt();  // check for ^C from user

        const int this_n_poss_gen = n_poss_gen[cross_group[ind]];
        double curloglik=0.0;

        // forward
        NumericMatrix alpha = forwardEquations2(genotypes(_,ind),
                                                init_vector[cross_group[ind]],
                                                emit_matrix[cross_group[ind]],
                                                step_matrix[cross_group[ind]],
                                                marker_index,
                                                poss_gen[cross_group[ind]]);

        bool curloglik_undef = true;
        for(int i=0; i<this_n_poss_gen; i++) {
            if(curloglik_undef) {
                curloglik_undef = false;
                curloglik = alpha(i,n_rf);
            }
            else {
                curloglik = addlog(curloglik, alpha(i, n_rf));
            }
        }
        loglik += curloglik;
    }

    if(verbose) {
        Rprintf("loglik = %.3f\n", loglik);
    }

    cur_rec_frac.attr("loglik") = loglik;
    if(cross_pu != cross) delete cross_pu;
    delete cross;
    return cur_rec_frac;
}


// Need same set of possible genotypes for all individuals,
// and same basic structure for transition matrix, but reorder transition matrix by founder order
// (for riself8 and riself16)
NumericVector est_map2_founderorder(const String crosstype,
                                    const IntegerMatrix& genotypes, // columns are individuals, rows are markers
                                    const IntegerMatrix& founder_geno, // columns are markers, rows are founder lines
                                    const bool is_X_chr,
                                    const LogicalVector& is_female,
                                    const IntegerMatrix& cross_info,
                                    const IntegerVector& cross_group, // [ignored here]
                                    const IntegerVector& unique_cross_group, // [ignored here]
                                    const NumericVector& rec_frac,
                                    const double error_prob,
                                    const int max_iterations,
                                    const double tol,
                                    const bool verbose)
{
    const int n_ind = genotypes.cols();
    const int n_mar = genotypes.rows();
    const int n_rf = n_mar-1;

    const double rf_tol = tol/1000.0; // smallest allowed recombination fraction
    const double rf_uptol = 0.999;    // largest allowed recombination fraction

    QTLCross* cross_pu = QTLCross::Create(crosstype);
    QTLCross* cross;
    if(cross_pu->crosstype != cross_pu->phase_known_crosstype) // get phase-known version of cross
        cross = QTLCross::Create(cross_pu->phase_known_crosstype);
    else cross = cross_pu;

    NumericVector cur_rec_frac(n_rf);
    NumericVector prev_rec_frac(clone(rec_frac));

    // marker index for forward/backward equations
    IntegerVector marker_index(n_mar);
    for(int i=0; i<n_mar; i++) marker_index[i] = i;

    // 3-d array to contain sum(gamma(il,ir)) for each interval
    const int n_gen = cross->ngen(is_X_chr);
    const int n_gen_sq = n_gen*n_gen;
    const int n_gen_sq_times_n_ind = n_gen_sq * n_ind;
    NumericVector full_gamma(n_gen_sq_times_n_ind * n_rf);

    // basic founder order
    const int n_founders = cross_info.rows();
    IntegerVector plain_founder_order(n_founders);
    for(int i=0; i<n_founders; i++) plain_founder_order[i] = i+1;

    // pre-calculate stuff; need separate ones for each unique value of is_female/cross_info
    const int max_obsgeno = max(genotypes);
    std::vector<NumericMatrix> emit_matrix = cross->calc_emitmatrix(error_prob, max_obsgeno,
                                                                    founder_geno,
                                                                    is_X_chr, false, plain_founder_order);
    NumericVector init_vector = cross->calc_initvector(is_X_chr, false, plain_founder_order);
    IntegerVector poss_gen = cross->possible_gen(is_X_chr, false, plain_founder_order);
    const int n_poss_gen = poss_gen.size();
    if(n_poss_gen != n_founders)
        throw std::range_error("no. possible genotypes != no. founders");

    // inverted index of founder orders
    IntegerMatrix founder_index(n_founders, n_ind);
    for(int ind=0; ind<n_ind; ind++)
        founder_index(_,ind) = invert_founder_index(cross_info(_,ind));

    bool converged = false; // flag for convergence
    for(int it=0; it<max_iterations; it++) {

        // transition matrix for current rec fracs
        std::vector<NumericMatrix> step_matrix = cross->calc_stepmatrix(prev_rec_frac, is_X_chr,
                                                                        false, plain_founder_order);

        // zero the full_gamma array
        full_gamma.fill(0.0);

        for(int ind=0; ind < n_ind; ind++) {

            Rcpp::checkUserInterrupt();  // check for ^C from user

            // reorder step matrix for this individual
            std::vector<NumericMatrix> ind_step_matrix(n_rf);
            for(int pos=0; pos<n_rf; pos++) {
                NumericMatrix this_step(n_poss_gen, n_poss_gen);
                for(int f1=0; f1<n_founders; f1++) {
                    this_step(f1,f1) = step_matrix[pos](f1,f1); // diagonal all the same
                    for(int f2=f1+1; f2<n_founders; f2++)
                        this_step(f1,f2) = this_step(f2,f1) =
                            step_matrix[pos](founder_index(f1,ind), founder_index(f2,ind));

                }
                ind_step_matrix[pos] = this_step;
            }

            // forward and backward equations
            NumericMatrix alpha = forwardEquations2(genotypes(_,ind), init_vector, emit_matrix, ind_step_matrix,
                                                    marker_index, poss_gen);
            NumericMatrix beta = backwardEquations2(genotypes(_,ind), init_vector, emit_matrix, ind_step_matrix,
                                                    marker_index, poss_gen);

            for(int pos=0; pos<n_rf; pos++) {
                // calculate gamma = log Pr(v1, v2, O)
                NumericMatrix gamma(n_poss_gen, n_poss_gen);
                double sum_gamma=0.0;
                bool sum_gamma_undef = true;
                for(int ir=0; ir<n_poss_gen; ir++) {
                    for(int il=0; il<n_poss_gen; il++) {
                        gamma(il,ir) = alpha(il,pos) + beta(ir,pos+1) +
                            emit_matrix[pos+1](genotypes(pos+1,ind),ir) +
                            ind_step_matrix[pos](il,ir);

                        if(sum_gamma_undef) {
                            sum_gamma_undef = false;
                            sum_gamma = gamma(il,ir);
                        }
                        else {
                            sum_gamma = addlog(sum_gamma, gamma(il,ir));
                        }
                    }
                }

                // add to full_gamma array of dim n_rf x n_ind x n_gen x n_gen
                const int offset = n_gen_sq_times_n_ind*pos + n_gen_sq*ind;
                for(int ir=0; ir<n_poss_gen; ir++) {
                    int gr_by_n_gen = (poss_gen[ir]-1)*n_gen;
                    for(int il=0; il<n_poss_gen; il++) {
                        int gl = poss_gen[il]-1;
                        full_gamma[offset + gr_by_n_gen + gl] += exp(gamma(il,ir) - sum_gamma);
                    }
                }
            } // loop over marker intervals

        } // loop over individuals

        // re-estimate rec'n fractions

        for(int pos=0; pos < n_rf; pos++) {
            // pull out the part for that position
            NumericVector sub_gamma(n_gen_sq_times_n_ind);
            std::copy(full_gamma.begin()+(n_gen_sq_times_n_ind*pos),
                      full_gamma.begin()+(n_gen_sq_times_n_ind*(pos+1)),
                      sub_gamma.begin());
            cur_rec_frac[pos] = cross->est_rec_frac(sub_gamma, is_X_chr, cross_info, n_gen);
        }

        // don't let rec fracs get too small
        for(int pos=0; pos<n_rf; pos++) {
            if(cur_rec_frac[pos] < rf_tol) cur_rec_frac[pos] = rf_tol;
            if(cur_rec_frac[pos] > rf_uptol) cur_rec_frac[pos] = rf_uptol;
        }

        if(verbose) {
            double maxdif = max(abs(prev_rec_frac - cur_rec_frac));
            Rprintf("%4d %.12f\n", it+1, maxdif);
        }

        // check convergence
        converged = true;
        for(int pos=0; pos<n_rf; pos++) {
            if(fabs(prev_rec_frac[pos] - cur_rec_frac[pos]) > tol*(cur_rec_frac[pos]+tol*100.0)) {
                converged = false;
                break;
            }
        }

        if(converged) break;

        prev_rec_frac = clone(cur_rec_frac);
    } // end loop over iterations


    if(!converged)
        r_warning("est_map reaching maximum iterations without converging");

    // transition matrix for current rec fracs
    std::vector<NumericMatrix> step_matrix = cross->calc_stepmatrix(cur_rec_frac, is_X_chr,
                                                                    false, plain_founder_order);

    // calculate log likelihood
    double loglik = 0.0;
    for(int ind=0; ind<n_ind; ind++) {
        Rcpp::checkUserInterrupt();  // check for ^C from user

        double curloglik=0.0;

        // reorder step matrix for this individual
        std::vector<NumericMatrix> ind_step_matrix(n_rf);
        for(int pos=0; pos<n_rf; pos++) {
            NumericMatrix this_step(n_poss_gen, n_poss_gen);
            for(int f1=0; f1<n_founders; f1++) {
                this_step(f1,f1) = step_matrix[pos](f1,f1); // diagonal all the same
                for(int f2=f1+1; f2<n_founders; f2++)
                    this_step(f1,f2) = this_step(f2,f1) =
                        step_matrix[pos](founder_index(f1,ind), founder_index(f2,ind));
            }
            ind_step_matrix[pos] = this_step;
        }

        // forward
        NumericMatrix alpha = forwardEquations2(genotypes(_,ind), init_vector, emit_matrix, ind_step_matrix,
                                                marker_index, poss_gen);

        bool curloglik_undef = true;
        for(int i=0; i<n_poss_gen; i++) {
            if(curloglik_undef) {
                curloglik_undef = false;
                curloglik = alpha(i,n_rf);
            }
            else {
                curloglik = addlog(curloglik, alpha(i, n_rf));
            }
        }
        loglik += curloglik;
    }

    if(verbose) {
        Rprintf("loglik = %.3f\n", loglik);
    }

    cur_rec_frac.attr("loglik") = loglik;
    if(cross_pu != cross) delete cross_pu;
    delete cross;
    return cur_rec_frac;
}
