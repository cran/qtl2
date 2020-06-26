// convert genotype probabilities to allele probabilities

#include "genoprob_to_alleleprob.h"
#include <math.h>
#include <Rcpp.h>
#include "cross.h"

// convert genotype probabilities to allele probabilities
// [[Rcpp::export(".genoprob_to_alleleprob")]]
NumericVector genoprob_to_alleleprob(const String& crosstype,
                                     const NumericVector& prob_array, // array as n_gen x n_ind x n_pos
                                     const bool is_x_chr)
{
    if(Rf_isNull(prob_array.attr("dim")))
        throw std::invalid_argument("prob_array should be a 3d array but has no dimension attribute");
    const IntegerVector& dim = prob_array.attr("dim");
    if(dim.size() != 3)
        throw std::invalid_argument("prob_array should be a 3d array of probabilities");
    const int n_gen = dim[0];
    const int n_ind = dim[1];
    const int n_pos = dim[2];
    const int ind_by_pos = n_ind*n_pos;

    QTLCross* cross = QTLCross::Create(crosstype);

    const NumericMatrix transform = cross->geno2allele_matrix(is_x_chr);
    const int n_allele = transform.cols()==0 ? n_gen : transform.cols();

    // space for result
    NumericVector result(ind_by_pos*n_allele); // no. alleles or n
    result.attr("dim") = Dimension(n_allele, n_ind, n_pos);

    if(n_allele == n_gen) { // no conversion needed
        std::copy(prob_array.begin(), prob_array.end(), result.begin());
    }
    else {
        if((int)transform.rows() != n_gen)
            throw std::invalid_argument("no. genotypes in prob_array doesn't match no. rows in transform matrix");

        for(int i=0, offset_gen=0, offset_allele=0;
            i < ind_by_pos;
            i++, offset_gen += n_gen, offset_allele += n_allele) {

            Rcpp::checkUserInterrupt();  // check for ^C from user

            for(int j=0; j < n_allele; j++) {
                for(int k=0; k<n_gen; k++) {
                    result[offset_allele+j] += prob_array[offset_gen+k]*transform(k,j);
                }
            }
        }
    }

    delete cross;
    return result;
}
