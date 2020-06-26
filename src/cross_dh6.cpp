// 6-way doubled haplotypes QTLCross class (for HMM)
// for maize MAGIC populations developed at the Wisconsin Crop Innovation Center

#include "cross_dh6.h"
#include <math.h>
#include <Rcpp.h>
#include "cross.h"
#include "cross_util.h"
#include "cross_do_util.h"
#include "r_message.h" // defines RQTL2_NODEBUG and r_message()

enum gen {A=1, H=2, B=3, notA=5, notB=4};

const bool DH6::check_geno(const int gen, const bool is_observed_value,
                           const bool is_x_chr, const bool is_female,
                           const IntegerVector& cross_info)
{
    // allow any value 0-5 for observed
    if(is_observed_value) {
        if(gen==0 || gen==A || gen==H || gen==B ||
           gen==notA || gen==notB) return true;
        else return false;
    }

    const int n_geno = 6;

    if(gen>= 1 && gen <= n_geno) return true;

    return false; // otherwise a problem
}

const double DH6::init(const int true_gen,
                       const bool is_x_chr, const bool is_female,
                       const IntegerVector& cross_info)
{
    #ifndef RQTL2_NODEBUG
    if(!check_geno(true_gen, false, is_x_chr, is_female, cross_info))
        throw std::range_error("genotype value not allowed");
    #endif

    return -log(6.0);
}

const double DH6::emit(const int obs_gen, const int true_gen, const double error_prob,
                       const IntegerVector& founder_geno, const bool is_x_chr,
                       const bool is_female, const IntegerVector& cross_info)
{
    #ifndef RQTL2_NODEBUG
    if(!check_geno(true_gen, false, is_x_chr, is_female, cross_info))
        throw std::range_error("genotype value not allowed");
    #endif

    if(obs_gen==0) return 0.0; // missing

    int f = founder_geno[true_gen-1]; // founder allele
    if(f!=1 && f!=3) return 0.0;      // founder missing -> no information

    if(f == obs_gen) return log(1.0 - error_prob);

    return log(error_prob); // genotyping error
}


const double DH6::step(const int gen_left, const int gen_right, const double rec_frac,
                            const bool is_x_chr, const bool is_female,
                            const IntegerVector& cross_info)
{
    #ifndef RQTL2_NODEBUG
    if(!check_geno(gen_left, false, is_x_chr, is_female, cross_info) ||
       !check_geno(gen_right, false, is_x_chr, is_female, cross_info))
        throw std::range_error("genotype value not allowed");
    #endif

    const int k = cross_info[0]; // number of generations

    double p = (1.0 + (5.0-6.0*rec_frac)*pow(1.0-rec_frac, (double)(k-2)))/6.0;

    if(gen_left == gen_right)
        return log(p);
    else
        return log(1.0-p) - log(5.0);
}

const IntegerVector DH6::possible_gen(const bool is_x_chr, const bool is_female,
                                       const IntegerVector& cross_info)
{
    int n_geno = 6;
    IntegerVector result(n_geno);

    for(int i=0; i<n_geno; i++) result[i] = i+1;
    return result;
}

const int DH6::ngen(const bool is_x_chr)
{
    return 6;
}

const int DH6::nalleles()
{
    return 6;
}


// check that cross_info conforms to expectation
const bool DH6::check_crossinfo(const IntegerMatrix& cross_info, const bool any_x_chr)
{
    bool result = true;
    const int n_row = cross_info.rows();
    const int n_col = cross_info.cols();
    // single column with the number of generations

    if(n_col != 1) {
        result = false;
        r_message("cross_info should have 1 column, indicating the number of generations");
        return result;
    }

    int n_missing=0;
    int n_invalid=0;
    for(int i=0; i<n_row; i++) {
        if(cross_info(i,0) == NA_INTEGER) ++n_missing;
        else if(cross_info(i,0) < 2) ++n_invalid;
    }
    if(n_missing > 0) {
        result = false;
        r_message("cross_info has missing values (it shouldn't)");
    }
    if(n_invalid > 0) {
        result = false;
        r_message("cross_info has invalid values; number of generations should be >= 2");
    }

    return result;
}


// check that founder genotype data has correct no. founders and markers
const bool DH6::check_founder_geno_size(const IntegerMatrix& founder_geno, const int n_markers)
{
    bool result=true;

    const int fg_mar = founder_geno.cols();
    const int fg_f   = founder_geno.rows();

    if(fg_mar != n_markers) {
        result = false;
        r_message("founder_geno has incorrect number of markers");
    }

    if(fg_f != 6) {
        result = false;
        r_message("founder_geno should have 6 founders");
    }

    return result;
}

// check that founder genotype data has correct values
const bool DH6::check_founder_geno_values(const IntegerMatrix& founder_geno)
{
    const int fg_mar = founder_geno.cols();
    const int fg_f   = founder_geno.rows();

    for(int f=0; f<fg_f; f++) {
        for(int mar=0; mar<fg_mar; mar++) {
            int fg = founder_geno(f,mar);
            if(fg != 0 && fg != 1 && fg != 3) {
                // at least one invalid value
                r_message("founder_geno contains invalid values; should be in {0, 1, 3}");
                return false;
            }
        }
    }

    return true;
}

const bool DH6::need_founder_geno()
{
    return true;
}

// geno_names from allele names
const std::vector<std::string> DH6::geno_names(const std::vector<std::string> alleles,
                                                const bool is_x_chr)
{
    if(alleles.size() < 6)
        throw std::range_error("alleles must have length 6");

    const int n_alleles = 6;

    std::vector<std::string> result(n_alleles);

    for(int i=0; i<n_alleles; i++)
        result[i] = alleles[i] + alleles[i];

    return result;
}


const int DH6::nrec(const int gen_left, const int gen_right,
                         const bool is_x_chr, const bool is_female,
                         const Rcpp::IntegerVector& cross_info)
{
    #ifndef RQTL2_NODEBUG
    if(!check_geno(gen_left, false, is_x_chr, is_female, cross_info) ||
       !check_geno(gen_right, false, is_x_chr, is_female, cross_info))
        throw std::range_error("genotype value not allowed");
    #endif

    if(gen_left == gen_right) return 0;
    else return 1;
}


// check whether X chr can be handled
const bool DH6::check_handle_x_chr(const bool any_x_chr)
{
    if(any_x_chr) {
        r_message("X chr ignored for 6-way doubled haploids.");
        return false;
    }

    return true; // most crosses can handle the X chr
}

// tailored est_map that pre-calculates transition matrices, etc
const NumericVector DH6::est_map2(const IntegerMatrix& genotypes,
                                      const IntegerMatrix& founder_geno,
                                      const bool is_X_chr,
                                      const LogicalVector& is_female,
                                      const IntegerMatrix& cross_info,
                                      const IntegerVector& cross_group,
                                      const IntegerVector& unique_cross_group,
                                      const NumericVector& rec_frac,
                                      const double error_prob,
                                      const int max_iterations,
                                      const double tol,
                                      const bool verbose)
{
    Rcpp::stop("est_map not yet implemented for 6-way doubled haploids.");

    // return vector of NAs
    const int n_rf = rec_frac.size();
    NumericVector result(n_rf);
    for(int i=0; i<n_rf; i++) result[i] = NA_REAL;
    return result ;
}
