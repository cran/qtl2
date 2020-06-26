// scan chromosome with linear mixed model for polygenic effect

#ifndef SCAN_PG_H
#define SCAN_PG_H

#include <Rcpp.h>

// LMM scan of a single chromosome with additive covariates and weights
//
// genoprobs = 3d array of genotype probabilities (individuals x genotypes x positions)
// pheno     = vector of numeric phenotypes
//             (no missing data allowed)
// addcovar  = additive covariates (an intercept, at least)
// eigenvec  = matrix of transposed eigenvectors of variance matrix
// weights   = vector of weights (really the SQUARE ROOT of the weights)
//
// output    = vector of log likelihood values
Rcpp::NumericVector scan_pg_onechr(const Rcpp::NumericVector& genoprobs,
                                   const Rcpp::NumericMatrix& pheno,
                                   const Rcpp::NumericMatrix& addcovar,
                                   const Rcpp::NumericMatrix& eigenvec,
                                   const Rcpp::NumericVector& weights,
                                   const double tol);

// LMM scan of a single chromosome with interactive covariates
// this version should be fast but requires more memory
// (since we first expand the genotype probabilities to probs x intcovar)
// and this one allows weights for the individuals (the same for all phenotypes)
//
// genoprobs = 3d array of genotype probabilities (individuals x genotypes x positions)
// pheno     = matrix with one column of numeric phenotypes
//             (no missing data allowed)
// addcovar  = additive covariates (an intercept, at least)
// intcovar  = interactive covariates (should also be included in addcovar)
// eigenvec  = matrix of transposed eigenvectors of variance matrix
// weights   = vector of weights (really the SQUARE ROOT of the weights)
//
// output    = vector of log likelihood values
Rcpp::NumericVector scan_pg_onechr_intcovar_highmem(const Rcpp::NumericVector& genoprobs,
                                                    const Rcpp::NumericMatrix& pheno,
                                                    const Rcpp::NumericMatrix& addcovar,
                                                    const Rcpp::NumericMatrix& intcovar,
                                                    const Rcpp::NumericMatrix& eigenvec,
                                                    const Rcpp::NumericVector& weights,
                                                    const double tol);

// LMM scan of a single chromosome with interactive covariates
// this version uses less memory but will be slower
// (since we need to work with each position, one at a time)
// and this one allows weights for the individuals (the same for all phenotypes)
//
// genoprobs = 3d array of genotype probabilities (individuals x genotypes x positions)
// pheno     = matrix with one column of numeric phenotypes
//             (no missing data allowed)
// addcovar  = additive covariates (an intercept, at least)
// intcovar  = interactive covariates (should also be included in addcovar)
// eigenvec  = matrix of transposed eigenvectors of variance matrix
// weights   = vector of weights (really the SQUARE ROOT of the weights)
//
// output    = vector of log likelihood values
Rcpp::NumericVector scan_pg_onechr_intcovar_lowmem(const Rcpp::NumericVector& genoprobs,
                                                   const Rcpp::NumericMatrix& pheno,
                                                   const Rcpp::NumericMatrix& addcovar,
                                                   const Rcpp::NumericMatrix& intcovar,
                                                   const Rcpp::NumericMatrix& eigenvec,
                                                   const Rcpp::NumericVector& weights,
                                                   const double tol);

#endif // SCAN_PG_H
