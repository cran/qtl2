// scan chromosome by LMM just to get coefficients
#ifndef SCAN1COEF_PG_H
#define SCAN1COEF_PG_H

#include <Rcpp.h>

// LMM scan of a single chromosome to calculate coefficients, with additive covariates
//
// genoprobs = 3d array of genotype probabilities (individuals x genotypes x positions)
// pheno     = vector of numeric phenotypes (individuals x 1)
//             (no missing data allowed)
// eigenvec  = eigenvectors from eigen decomposition of kinship matrix
// addcovar  = additive covariates
// weights   = vector of weights (really the SQUARE ROOT of the weights)
//
// output    = matrix of coefficients (genotypes x positions)
Rcpp::NumericMatrix scancoef_pg_addcovar(const Rcpp::NumericVector& genoprobs,
                                         const Rcpp::NumericVector& pheno,
                                         const Rcpp::NumericMatrix& addcovar,
                                         const Rcpp::NumericMatrix& eigenvec,
                                         const Rcpp::NumericVector& weights,
                                         const double tol);

// LMM scan of a single chromosome to calculate coefficients, with interactive covariates
//
// genoprobs = 3d array of genotype probabilities (individuals x genotypes x positions)
// pheno     = vector of numeric phenotypes (individuals x 1)
//             (no missing data allowed)
// addcovar  = additive covariates
// intcovar  = interactive covariates (should also be included in addcovar)
// eigenvec  = eigenvectors from eigen decomposition of kinship matrix
// weights   = vector of weights (really the SQUARE ROOT of the weights)
//
// output    = matrix of coefficients (genotypes x positions)
Rcpp::NumericMatrix scancoef_pg_intcovar(const Rcpp::NumericVector& genoprobs,
                                         const Rcpp::NumericVector& pheno,
                                         const Rcpp::NumericMatrix& addcovar,
                                         const Rcpp::NumericMatrix& intcovar,
                                         const Rcpp::NumericMatrix& eigenvec,
                                         const Rcpp::NumericVector& weights,
                                         const double tol);




// LMM scan of a single chromosome to calculate coefficients, with additive covariates
//
// genoprobs = 3d array of genotype probabilities (individuals x genotypes x positions)
// pheno     = vector of numeric phenotypes (individuals x 1)
//             (no missing data allowed)
// addcovar  = additive covariates
// eigenvec  = eigenvectors from eigen decomposition of kinship matrix
// weights   = vector of weights (really the SQUARE ROOT of the weights)
//
// output    = list of two matrices, of coefficients and SEs (each genotypes x positions)
Rcpp::List scancoefSE_pg_addcovar(const Rcpp::NumericVector& genoprobs,
                                  const Rcpp::NumericVector& pheno,
                                  const Rcpp::NumericMatrix& addcovar,
                                  const Rcpp::NumericMatrix& eigenvec,
                                  const Rcpp::NumericVector& weights,
                                  const double tol);


// LMM scan of a single chromosome to calculate coefficients, with interactive covariates
//
// genoprobs = 3d array of genotype probabilities (individuals x genotypes x positions)
// pheno     = vector of numeric phenotypes (individuals x 1)
//             (no missing data allowed)
// addcovar  = additive covariates
// intcovar  = interactive covariates (should also be included in addcovar)
// eigenvec  = eigenvectors from eigen decomposition of kinship matrix
// weights   = vector of weights (really the SQUARE ROOT of the weights)
//
// output    = list of two matrices, of coefficients and SEs (each genotypes x positions)
Rcpp::List scancoefSE_pg_intcovar(const Rcpp::NumericVector& genoprobs,
                                  const Rcpp::NumericVector& pheno,
                                  const Rcpp::NumericMatrix& addcovar,
                                  const Rcpp::NumericMatrix& intcovar,
                                  const Rcpp::NumericMatrix& eigenvec,
                                  const Rcpp::NumericVector& weights,
                                  const double tol);
#endif // SCAN1COEF_PG_H
