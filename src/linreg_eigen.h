// linear regression via RcppEigen
#ifndef LINREG_EIGEN_H
#define LINREG_EIGEN_H

#include <RcppEigen.h>

// calc X'X
Eigen::MatrixXd calc_XpX(const Eigen::MatrixXd& A);

// least squares by "LLt" Cholesky decomposition
// needs to be full rank
Rcpp::List fit_linreg_eigenchol(const Rcpp::NumericMatrix& X,
                                const Rcpp::NumericVector& y,
                                const bool se);

// least squares by "LLt" Cholesky decomposition
// return just the coefficients
Rcpp::NumericVector calc_coef_linreg_eigenchol(const Rcpp::NumericMatrix& X,
                                               const Rcpp::NumericVector& y);

// least squares by "LLt" Cholesky decomposition
// this returns the coefficients and SEs
Rcpp::List calc_coefSE_linreg_eigenchol(const Rcpp::NumericMatrix& X,
                                        const Rcpp::NumericVector& y,
                                        const double tol);

// least squares by "LLt" Cholesky decomposition
// return just the residual sum of squares
// needs to be full rank
double calc_rss_eigenchol(const Rcpp::NumericMatrix& X,
                          const Rcpp::NumericVector& y);

// least squares by "LLt" Cholesky decomposition
// return just the fitted values
Rcpp::NumericVector calc_fitted_linreg_eigenchol(const Rcpp::NumericMatrix& X,
                                                 const Rcpp::NumericVector& y);

// least squares by QR decomposition with column pivoting
Rcpp::List fit_linreg_eigenqr(const Rcpp::NumericMatrix& X,
                              const Rcpp::NumericVector& y,
                              const bool se,
                              const double tol);

// least squares by QR decomposition with column pivoting
// this just returns the coefficients
Rcpp::NumericVector calc_coef_linreg_eigenqr(const Rcpp::NumericMatrix& X,
                                             const Rcpp::NumericVector& y,
                                             const double tol);

// least squares by QR decomposition with column pivoting
// this returns the coefficients and SEs
Rcpp::List calc_coefSE_linreg_eigenqr(const Rcpp::NumericMatrix& X,
                                      const Rcpp::NumericVector& y,
                                      const double tol);

// least squares by QR decomposition with column pivoting
// return just the residual sum of squares
double calc_rss_eigenqr(const Rcpp::NumericMatrix& X,
                        const Rcpp::NumericVector& y,
                        const double tol);

// least squares by QR decomposition with column pivoting
// return just the fitted values
Rcpp::NumericVector calc_fitted_linreg_eigenqr(const Rcpp::NumericMatrix& X,
                                               const Rcpp::NumericVector& y,
                                               const double tol);

// least squares by "LLt" Cholesky decomposition, with matrix Y
// return vector of RSS
Rcpp::NumericVector calc_mvrss_eigenchol(const Rcpp::NumericMatrix& X,
                                         const Rcpp::NumericMatrix& Y);

// least squares by QR decomposition with column pivoting, with matrix Y
// return vector of RSS
Rcpp::NumericVector calc_mvrss_eigenqr(const Rcpp::NumericMatrix& X,
                                       const Rcpp::NumericMatrix& Y,
                                       const double tol);

// least squares by "LLt" Cholesky decomposition, with matrix Y
// return matrix of residuals
Rcpp::NumericMatrix calc_resid_eigenchol(const Rcpp::NumericMatrix& X,
                                         const Rcpp::NumericMatrix& Y);

// least squares by QR decomposition with column pivoting, with matrix Y
// return matrix of residuals
Rcpp::NumericMatrix calc_resid_eigenqr(const Rcpp::NumericMatrix& X,
                                       const Rcpp::NumericMatrix& Y,
                                       const double tol);

#endif // LINREG_EIGEN_H
