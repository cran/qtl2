// QTLCross class for general k-way recombinant inbred lines or doubled haploids

#ifndef CROSS_GENRIL_H
#define CROSS_GENRIL_H

#include <Rcpp.h>
#include "cross.h"

class GENRIL : public QTLCross
{
 public:
    int n_founders;

    GENRIL(int nf){
        crosstype = "genril";
        phase_known_crosstype = "genril";
        if(nf < 2) {
            throw std::range_error("general RIL should have >= 2 founders");
        }
        n_founders = nf;
    };
    ~GENRIL(){};

    const bool check_geno(const int gen, const bool is_observed_value,
                          const bool is_x_chr, const bool is_female, const Rcpp::IntegerVector& cross_info);

    const double init(const int true_gen,
                      const bool is_x_chr, const bool is_female, const Rcpp::IntegerVector& cross_info);
    const double emit(const int obs_gen, const int true_gen, const double error_prob,
                      const Rcpp::IntegerVector& founder_geno, const bool is_x_chr,
                      const bool is_female, const Rcpp::IntegerVector& cross_info);
    const double step(const int gen_left, const int gen_right, const double rec_frac,
                      const bool is_x_chr, const bool is_female, const Rcpp::IntegerVector& cross_info);

    const Rcpp::IntegerVector possible_gen(const bool is_x_chr, const bool is_female, const Rcpp::IntegerVector& cross_info);

    const int ngen(const bool is_x_chr);
    const int nalleles();

    const bool check_crossinfo(const Rcpp::IntegerMatrix& cross_info, const bool any_x_chr);

    const bool check_founder_geno_size(const Rcpp::IntegerMatrix& founder_geno, const int n_markers);
    const bool check_founder_geno_values(const Rcpp::IntegerMatrix& founder_geno);
    const bool need_founder_geno();

    const std::vector<std::string> geno_names(const std::vector<std::string> alleles, const bool is_x_chr);

    const int nrec(const int gen_left, const int gen_right,
                   const bool is_x_chr, const bool is_female,
                   const Rcpp::IntegerVector& cross_info);

    // check whether X chr can be handled
    const bool check_handle_x_chr(const bool any_x_chr);

    // tailored est_map that pre-calculates transition matrices, etc
    const Rcpp::NumericVector est_map2(const Rcpp::IntegerMatrix& genotypes,
                                       const Rcpp::IntegerMatrix& founder_geno,
                                       const bool is_X_chr,
                                       const Rcpp::LogicalVector& is_female,
                                       const Rcpp::IntegerMatrix& cross_info,
                                       const Rcpp::IntegerVector& cross_group,
                                       const Rcpp::IntegerVector& unique_cross_group,
                                       const Rcpp::NumericVector& rec_frac,
                                       const double error_prob,
                                       const int max_iterations,
                                       const double tol,
                                       const bool verbose);

};

// step for general chromosome, used for both general RIL and general AIL
const double step_genchr(const int gen_left, const int gen_right, const double rec_frac,
                         const bool is_xchr, const IntegerVector& cross_info, const int n_founders);

#endif // CROSS_GENRIL_H
