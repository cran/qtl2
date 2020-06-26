// calculate conditional genotype probabilities given multipoint marker data

#include "hmm_calcgenoprob.h"
#include <math.h>
#include <Rcpp.h>
#include "cross.h"
#include "hmm_util.h"
#include "hmm_forwback.h"

// calculate conditional genotype probabilities given multipoint marker data
// [[Rcpp::export(".calc_genoprob")]]
NumericVector calc_genoprob(const String& crosstype,
                            const IntegerMatrix& genotypes, // columns are individuals, rows are markers
                            const IntegerMatrix& founder_geno, // columns are markers, rows are founder lines
                            const bool is_X_chr,
                            const LogicalVector& is_female, // length n_ind
                            const IntegerMatrix& cross_info, // columns are individuals
                            const NumericVector& rec_frac,   // length nrow(genotypes)-1
                            const IntegerVector& marker_index, // length nrow(genotypes)
                            const double error_prob)
{
    const int n_ind = genotypes.cols();
    const int n_pos = marker_index.size();
    const int n_mar = genotypes.rows();

    QTLCross* cross = QTLCross::Create(crosstype);

    // check inputs
    if(is_female.size() != n_ind)
        throw std::range_error("length(is_female) != ncol(genotypes)");
    if(cross_info.cols() != n_ind)
        throw std::range_error("ncols(cross_info) != ncol(genotypes)");
    if(rec_frac.size() != n_pos-1)
        throw std::range_error("length(rec_frac) != length(marker_index)-1");

    if(error_prob < 0.0 || error_prob > 1.0)
        throw std::range_error("error_prob out of range");

    for(int i=0; i<rec_frac.size(); i++) {
        if(rec_frac[i] < 0 || rec_frac[i] > 0.5)
            throw std::range_error("rec_frac must be >= 0 and <= 0.5");
    }
    if(!cross->check_founder_geno_size(founder_geno, n_mar))
        throw std::range_error("founder_geno is not the right size");
    // end of checks

    const int n_gen = cross->ngen(is_X_chr);
    const int matsize = n_gen*n_ind; // size of genotype x individual matrix
    NumericVector genoprobs(matsize*n_pos);

    for(int ind=0; ind<n_ind; ind++) {

        Rcpp::checkUserInterrupt();  // check for ^C from user

        // possible genotypes for this individual
        IntegerVector poss_gen = cross->possible_gen(is_X_chr, is_female[ind], cross_info(_,ind));
        const int n_poss_gen = poss_gen.size();

        // forward/backward equations
        NumericMatrix alpha = forwardEquations(cross, genotypes(_,ind), founder_geno, is_X_chr, is_female[ind],
                                               cross_info(_,ind), rec_frac, marker_index, error_prob,
                                               poss_gen);
        NumericMatrix beta = backwardEquations(cross, genotypes(_,ind), founder_geno, is_X_chr, is_female[ind],
                                               cross_info(_,ind), rec_frac, marker_index, error_prob,
                                               poss_gen);

        // calculate genotype probabilities
        for(int pos=0, matindex=n_gen*ind; pos<n_pos; pos++, matindex += matsize) {
            int g = poss_gen[0]-1;
            double sum_at_pos = genoprobs[matindex+g] = alpha(0,pos) + beta(0,pos);
            for(int i=1; i<n_poss_gen; i++) {
                int g = poss_gen[i]-1;
                double val = genoprobs[matindex+g] = alpha(i,pos) + beta(i,pos);
                sum_at_pos = addlog(sum_at_pos, val);
            }
            for(int i=0; i<n_poss_gen; i++) {
                int g = poss_gen[i]-1;
                genoprobs[matindex+g] = exp(genoprobs[matindex+g] - sum_at_pos);
            }
        }
    } // loop over individuals

    genoprobs.attr("dim") = Dimension(n_gen, n_ind, n_pos);
    delete cross;
    return genoprobs;
}
