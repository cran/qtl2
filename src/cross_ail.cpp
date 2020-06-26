// AIL QTLCross class (for HMM)

#include "cross_ail.h"
#include <math.h>
#include <Rcpp.h>
#include "cross.h"
#include "r_message.h" // defines RQTL2_NODEBUG and r_message()

enum gen {AA=1, AB=2, BB=3, notA=5, notB=4,
          AAX=1, ABX=2, BBX=3, AY=4, BY=5};

const bool AIL::check_geno(const int gen, const bool is_observed_value,
                           const bool is_x_chr, const bool is_female, const IntegerVector& cross_info)
{
    // allow any value 0-5 for observed
    if(is_observed_value) {
        if(gen==0 || gen==AA || gen==AB || gen==BB ||
           gen==notA || gen==notB) return true;
        else return false;
    }

    if(is_x_chr) {
        if(is_female) {
            if(gen==AAX || gen==ABX || gen==BBX) return true;
        }
        else if(gen==AY || gen==BY) return true;
    }
    else if(gen==AA || gen==AB || gen==BB) return true;

    return false; // otherwise a problem
}

const double AIL::init(const int true_gen,
                       const bool is_x_chr, const bool is_female,
                       const IntegerVector& cross_info)
{
    #ifndef RQTL2_NODEBUG
    if(!check_geno(true_gen, false, is_x_chr, is_female, cross_info))
        throw std::range_error("genotype value not allowed");
    #endif

    const int n_gen = cross_info[0];
    int dir;
    if(cross_info.size() > 1) {
       dir = cross_info[1];
    }
    else { // assume balanced case
        dir = 2;
    }

    if(is_x_chr) {
        if(dir==2) { // balanced case
            if(is_female) {
                if(true_gen==AB) return log(0.5);
                else return log(0.25);
            }
            else { // male
                return log(0.5);
            }
        }
        else { // AxB or BxA
            // frequency of A in AxB in males is (2/3) + (1/3)*(-1/2)^(s-1)
            double logm, logf, log1mm, log1mf;
            if(n_gen % 2 == 1) { // s is odd
                logf = log(2.0/3.0) + log1p( -exp( -((double)(n_gen+1) * log(2.0)) ) );
                logm = log(2.0/3.0) + log1pexp( -((double)(n_gen) * log(2.0)) );
            }
            else {
                logf = log(2.0/3.0) + log1pexp( -((double)(n_gen+1) * log(2.0)) );
                logm = log(2.0/3.0) + log1p( -exp( -((double)(n_gen) * log(2.0)) ) );
            }

            if(dir == 0) { // AxB
                log1mf = log1p(-exp(logf));
                log1mm = log1p(-exp(logm));
            }
            else { // BxA, take 1-p
                log1mf = logf;
                log1mm = logm;
                logf = log1p(-exp(logf));
                logm = log1p(-exp(logm));
            }

            if(is_female) {
                if(true_gen == AAX) return 2.0*logf;
                else if(true_gen == ABX) return log(2.0) + logf + log1mf;
                else return 2.0*log1mf;
            }
            else { // male
                if(true_gen==AY) return logm;
                else return log1mm;
            }
        }
    }
    else {
        if(true_gen==AB) return log(0.5);
        else return log(0.25);
    }
}

const double AIL::emit(const int obs_gen, const int true_gen, const double error_prob,
                       const IntegerVector& founder_geno, const bool is_x_chr,
                       const bool is_female, const IntegerVector& cross_info)
{
    #ifndef RQTL2_NODEBUG
    if(!check_geno(true_gen, false, is_x_chr, is_female, cross_info))
        throw std::range_error("genotype value not allowed");
    #endif

    if(obs_gen==0 || !check_geno(obs_gen, true, is_x_chr, is_female, cross_info))
       return 0.0; // missing or invalid

    if(is_female || !is_x_chr) { // female X chromosome just like autosome
        if(true_gen==AA) {
            if(obs_gen==AA) return log(1.0-error_prob);
            if(obs_gen==AB || obs_gen==BB) return log(error_prob/2.0);
            if(obs_gen==notB) return log(1.0-error_prob/2.0);
            if(obs_gen==notA) return log(error_prob);
        }
        if(true_gen==AB) {
            if(obs_gen==AB) return log(1.0-error_prob);
            if(obs_gen==AA || obs_gen==BB) return log(error_prob/2.0);
            if(obs_gen==notB || obs_gen==notA) return log(1.0-error_prob/2.0);
        }
        if(true_gen==BB) {
            if(obs_gen==BB) return log(1.0-error_prob);
            if(obs_gen==AB || obs_gen==AA) return log(error_prob/2.0);
            if(obs_gen==notA) return log(1.0-error_prob/2.0);
            if(obs_gen==notB) return log(error_prob);
        }
    }
    else { // males
        if(true_gen==AY) {
            if(obs_gen==AA || obs_gen==notB) return log(1.0-error_prob);
            if(obs_gen==BB || obs_gen==notA) return log(error_prob);
            return 0.0; // treat everything else as missing
        }
        if(true_gen==BY) {
            if(obs_gen==BB || obs_gen==notA) return log(1.0-error_prob);
            if(obs_gen==AA || obs_gen==notB) return log(error_prob);
            return 0.0; // treat everything else as missing
        }
    }

    return NA_REAL; // shouldn't get here
}


const double AIL::step(const int gen_left, const int gen_right, const double rec_frac,
                       const bool is_x_chr, const bool is_female,
                       const IntegerVector& cross_info)
{
    #ifndef RQTL2_NODEBUG
    if(!check_geno(gen_left, false, is_x_chr, is_female, cross_info) ||
       !check_geno(gen_right, false, is_x_chr, is_female, cross_info))
        throw std::range_error("genotype value not allowed");
    #endif

    const int n_gen = cross_info[0];
    int dir;
    if(cross_info.size() > 1) {
       dir = cross_info[1];
    }
    else { // assume balanced case
        dir = 2;
    }

    if(is_x_chr) {
        if(dir == 2) { // balanced case
            double z = sqrt((1.0-rec_frac)*(9.0-rec_frac));
            double w = (1.0 - rec_frac + z)/4.0;
            double y = (1.0 - rec_frac - z)/4.0; // make positive
            double Rm, Rf;
            double k = (double)(n_gen - 2);
            double wk = pow(w,k);
            double yk = pow(y,k);
            Rm = 1 - 0.25*(2.0 + (1.0-2.0*rec_frac) * (wk + yk) +
                           (3.0 - 5.0*rec_frac + 2.0*rec_frac*rec_frac)/z * (wk - yk));
            Rf = 1 - 0.25*(2.0 + (1.0-2.0*rec_frac) * (wk + yk) +
                           (3.0 - 6.0*rec_frac + rec_frac*rec_frac)/z * (wk - yk));

            if(is_female) {
                switch(gen_left) {
                case AA:
                    switch(gen_right) {
                    case AA: return 2.0*log1p(-Rf);
                    case AB: return log(2.0)+log1p(-Rf)+log(Rf);
                    case BB: return 2.0*log(Rf);
                    }
                case AB:
                    switch(gen_right) {
                    case AA: case BB: return log(Rf)+log1p(-Rf);
                    case AB: return log(Rf*Rf + (1.0-Rf)*(1.0-Rf));
                    }
                case BB:
                    switch(gen_right) {
                    case AA: return 2.0*log(Rf);
                    case AB: return log(2.0)+log1p(-Rf)+log(Rf);
                    case BB: return 2.0*log1p(-Rf);
                    }
                }
            }
            else { // male
                if(gen_left == gen_right) return log1p(-Rm);
                return log(Rm);
            }
        }
        else { // 0 = AxB; 1 = BxA
            // calculate frequency of AA haplotype in males and females
            double m11prev = 1.0;
            double f11prev = 0.5;
            double m11=1.0, f11=0.5, qp, qpp;
            for(int i=2; i<=n_gen; i++) {
                qpp = (2.0/3.0) + (1.0/3.0)*pow(-0.5, (double)(i-3));
                qp  = (2.0/3.0) + (1.0/3.0)*pow(-0.5, (double)(i-2));
                m11 = (1.0-rec_frac)*f11prev + rec_frac*qp*qpp;
                f11 = m11prev/2.0 + (1.0-rec_frac)/2.0*f11prev + (rec_frac/2.0)*qp*qpp;
                m11prev = m11;
                f11prev = f11;
            }

            if(is_female) {
                // allele frequencies at this generation
                double qf = (2.0/3.0) + (1.0/3.0)*pow(-0.5, (double)n_gen);
                // conditional probabilities along random haplotypes
                double f1to1 = f11/qf;
                double f1to2 = 1.0 - f1to1;
                double f2to1 = (qf - f11)/(1.0-qf);
                double f2to2 = 1.0 - f2to1;

                if(dir == 0) { // AxB
                    switch(gen_left) {
                    case AA:
                        switch(gen_right) {
                        case AA: return 2.0*log(f1to1);
                        case AB: return log(2.0)+log(f1to1)+log(f1to2);
                        case BB: return 2.0*log(f1to2);
                        }
                    case AB:
                        switch(gen_right) {
                        case AA: return log(f1to1)+log(f2to1);
                        case AB: return log(f1to1*f2to2 + f1to2*f2to1);
                        case BB: return log(f1to2)+log(f2to2);
                        }
                    case BB:
                        switch(gen_right) {
                        case AA: return 2.0*log(f2to1);
                        case AB: return log(2.0)+log(f2to2)+log(f2to1);
                        case BB: return 2.0*log(f2to2);
                        }
                    }
                }
                else { // BxA
                    switch(gen_left) {
                    case BB:
                        switch(gen_right) {
                        case BB: return 2.0*log(f1to1);
                        case AB: return log(2.0)+log(f1to1)+log(f1to2);
                        case AA: return 2.0*log(f1to2);
                        }
                    case AB:
                        switch(gen_right) {
                        case BB: return log(f1to1)+log(f2to1);
                        case AB: return log(f1to1*f2to2 + f1to2*f2to1);
                        case AA: return log(f1to2)+log(f2to2);
                        }
                    case AA:
                        switch(gen_right) {
                        case BB: return 2.0*log(f2to1);
                        case AB: return log(2.0)+log(f2to2)+log(f2to1);
                        case AA: return 2.0*log(f2to2);
                        }
                    }
                }
            }
            else {
                // allele frequencies at this generation
                double qm = (2.0/3.0) + (1.0/3.0)*pow(-0.5, (double)(n_gen-1));
                // conditional probabilities along random haplotypes
                double m1to1 = m11/qm;
                double m1to2 = 1.0 - m1to1;
                double m2to1 = (qm - m11)/(1.0-qm);
                double m2to2 = 1.0 - m2to1;

                if(dir == 0) { // AxB
                    switch(gen_left) {
                    case AY:
                        switch(gen_right) {
                        case AY: return log(m1to1);
                        case BY: return log(m1to2);
                        }
                    case BY:
                        switch(gen_right) {
                        case AY: return log(m2to1);
                        case BY: return log(m2to2);
                        }
                    }
                }
                else { // BxA; change A->B and B->A
                    switch(gen_left) {
                    case AY:
                        switch(gen_right) {
                        case AY: return log(m2to2);
                        case BY: return log(m2to1);
                        }
                    case BY:
                        switch(gen_right) {
                        case AY: return log(m1to2);
                        case BY: return log(m1to1);
                        }
                    }
                }


            }
        }
    }
    else { // autosome
        // cross_info[0] is the number of generations
        // R = 0.5*[ 1 - (1-2r)*(1-r)^(s-2) ]
        double tmp = (1-2.0*rec_frac)*pow(1.0-rec_frac, (double)(n_gen-2));
        double logR = -log(2.0) + log1p(-tmp);
        double log1mR = -log(2.0) + log1p(tmp);

        switch(gen_left) {
        case AA:
            switch(gen_right) {
            case AA: return 2.0*log1mR;
            case AB: return log(2.0)+log1mR+logR;
            case BB: return 2.0*logR;
            }
        case AB:
            switch(gen_right) {
            case AA: case BB: return logR+log1mR;
            case AB: return log(exp(2.0*logR) + exp(2.0*log1mR));
            }
        case BB:
            switch(gen_right) {
            case AA: return 2.0*logR;
            case AB: return log(2.0)+log1mR+logR;
            case BB: return 2.0*log1mR;
            }
        }
    }

    return NA_REAL; // shouldn't get here
}

const IntegerVector AIL::possible_gen(const bool is_x_chr, const bool is_female,
                                      const IntegerVector& cross_info)
{
    if(is_x_chr && !is_female) { // male X chromosome
        IntegerVector result = IntegerVector::create(AY,BY);
        return result;
    }
    else { // autosome
        IntegerVector result = IntegerVector::create(AA,AB,BB);
        return result;
    }
}

const int AIL::ngen(const bool is_x_chr)
{
    if(is_x_chr) return 5;
    return 3;
}

const NumericMatrix AIL::geno2allele_matrix(const bool is_x_chr)
{
    if(is_x_chr) {
        NumericMatrix result(5,2);
        result(0,0) = 1.0;               // AA female
        result(1,0) = result(1,1) = 0.5; // AB female
        result(2,1) = 1.0;               // BB female

        result(3,0) = 1.0; // AY male
        result(4,1) = 1.0; // BY male

        return result;
    }
    else {
        NumericMatrix result(3,2);
        result(0,0) = 1.0;
        result(1,0) = result(1,1) = 0.5;
        result(2,1) = 1.0;

        return result;
    }
}

// check that sex conforms to expectation
const bool AIL::check_is_female_vector(const LogicalVector& is_female, const bool any_x_chr)
{
    bool result = true;
    const int n = is_female.size();
    if(!any_x_chr) { // all autosomes
        if(n > 0) {
            // not needed here, but don't call it an error
            result = true;
        }
    }
    else { // X chr included
        if(n == 0) {
            result = false;
            r_message("is_female not provided, but needed to handle X chromosome");
        }
        else {
            int n_missing = 0;
            for(int i=0; i<n; i++)
                if(is_female[i] == NA_LOGICAL) ++n_missing;
            if(n_missing > 0) {
                result = false;
                r_message("is_female contains missing values (it shouldn't)");
            }
        }
    }
    return result;
}

// check that cross_info conforms to expectation
const bool AIL::check_crossinfo(const IntegerMatrix& cross_info, const bool any_x_chr)
{
    bool result = true;
    const int n_row = cross_info.rows();
    const int n_col = cross_info.cols();
    // first column is number of generations (needed no matter what; values should be >= 2)
    // second column is 0=AxB, 1=BxA, 2=balanced (needed for X chromosome)

    if(n_col == 0) {
        result = false;
        r_message("cross_info should at least have one column, with no. generations");
        return result;
    }

    int n_missing=0;
    int n_invalid=0;
    for(int i=0; i<n_row; i++) {
        if(cross_info[i] == NA_INTEGER) ++n_missing;
        else if(cross_info[i] < 2) ++n_invalid;
    }
    if(n_missing > 0) {
        result = false;
        r_message("1st column in cross_info has missing values (it shouldn't)");
    }
    if(n_invalid > 0) {
        result = false;
        r_message("1st column in cross_info has invalid values; no. generations should be >= 2");
    }

    if(n_col == 1 && any_x_chr) {
        result = false;
        r_message("cross_info should have at two columns (no. generations and cross direction)");
    }

    if(n_col > 1) {
        if(n_col > 2) {
            result = false;
            r_message("cross_info should have no more than 2 columns (no. generations and cross direction)");
        }

        int n_missing = 0;
        int n_invalid = 0;
        for(int i=0; i<n_row; i++) {
            if(cross_info[i+n_row] == NA_INTEGER) ++n_missing;
            else if(cross_info[i+n_row] != 0 &&
                    cross_info[i+n_row] != 1 &&
                    cross_info[i+n_row] != 2) {
                ++n_invalid;
            }
        }
        if(n_missing > 0) {
            result = false;
            r_message("2nd column in cross_info contains missing values (it shouldn't)");
        }

        if(n_invalid > 0) {
            result = false;
            r_message("2nd column in cross_info contains invalid values; should be 0, 1, or 2.");
        }
    }
    return result;
}

// geno_names from allele names
const std::vector<std::string> AIL::geno_names(const std::vector<std::string> alleles,
                                               const bool is_x_chr)
{
    if(alleles.size() < 2)
        throw std::range_error("alleles must have length 2");

    if(is_x_chr) {
        std::vector<std::string> result(5);
        result[0] = alleles[0] + alleles[0];
        result[1] = alleles[0] + alleles[1];
        result[2] = alleles[1] + alleles[1];
        result[3] = alleles[0] + "Y";
        result[4] = alleles[1] + "Y";
        return result;
    }
    else {
        std::vector<std::string> result(3);
        result[0] = alleles[0] + alleles[0];
        result[1] = alleles[0] + alleles[1];
        result[2] = alleles[1] + alleles[1];
        return result;
    }
}

// used in counting crossovers in count_xo.R and locate_xo.R
const int AIL::nrec(const int gen_left, const int gen_right,
                    const bool is_x_chr, const bool is_female,
                    const Rcpp::IntegerVector& cross_info)
{
    #ifndef RQTL2_NODEBUG
    if(!check_geno(gen_left, false, is_x_chr, is_female, cross_info) ||
       !check_geno(gen_right, false, is_x_chr, is_female, cross_info))
        throw std::range_error("genotype value not allowed");
    #endif

    if(!is_x_chr || is_female) {
        switch(gen_left) {
        case 1:
            switch(gen_right) {
            case 1: return(0);
            case 2: return(1);
            case 3: return(2);
            }
        case 2:
            switch(gen_right) {
            case 1: case 3: return(1);
            case 2: return(0);
            }
        case 3:
            switch(gen_right) {
            case 1: return(2);
            case 2: return(1);
            case 3: return(0);
            }
        }
    }
    else { // X chromosome, males (possible values are 4 or 5)
        if(gen_left == gen_right) return(0);
        else return(1);
    }

    return(NA_INTEGER);
}

// not implemented
const NumericVector AIL::est_map2(const IntegerMatrix& genotypes,
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
    Rcpp::stop("est_map not yet implemented for AILs.");

    // return vector of NAs
    const int n_rf = rec_frac.size();
    NumericVector result(n_rf);
    for(int i=0; i<n_rf; i++) result[i] = NA_REAL;
    return result ;
}
