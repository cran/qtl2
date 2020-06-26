// RI by sib-mating QTLCross class (for HMM)

#ifndef CROSS_RISIB_H
#define CROSS_RISIB_H

#include <Rcpp.h>
#include "cross.h"

class RISIB : public QTLCross
{
 public:
    RISIB(){
        crosstype = "risib";
        phase_known_crosstype = "risib";
    };

    ~RISIB(){};

    const double init(const int true_gen,
                      const bool is_x_chr, const bool is_female, const Rcpp::IntegerVector& cross_info);

    const double step(const int gen_left, const int gen_right, const double rec_frac,
                      const bool is_x_chr, const bool is_female, const Rcpp::IntegerVector& cross_info);

    const double est_rec_frac(const Rcpp::NumericVector& gamma, const bool is_x_chr,
                              const Rcpp::IntegerMatrix& cross_info, const int n_gen);

    const bool check_crossinfo(const Rcpp::IntegerMatrix& cross_info, const bool any_x_chr);

};

#endif // CROSS_RISIB_H
