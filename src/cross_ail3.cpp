// AIL3 (3-way advanced intercross lines) QTLCross class (for HMM)
// (assuming all F1 hybrids formed followed by random mating with large population)

#include "cross_ail3.h"
#include <math.h>
#include <Rcpp.h>
#include "cross.h"
#include "cross_util.h" // mpp_encode_alleles and mpp_decode_geno
#include "r_message.h" // defines RQTL2_NODEBUG and r_message()

enum gen {AA=1, AB=2, BB=3, notA=5, notB=4,
          A=1, H=2, B=3};

const bool AIL3::check_geno(const int gen, const bool is_observed_value,
                            const bool is_x_chr, const bool is_female, const IntegerVector& cross_info)
{
    // allow any value 0-5 for observed
    if(is_observed_value) {
        if(gen==0 || gen==AA || gen==AB || gen==BB ||
           gen==notA || gen==notB) return true;
        else return false;
    }

    const int n_genoA = 6;
    const int n_genoX = 9;

    if(!is_x_chr || is_female) {// autosome or female X
        if(gen >= 1 && gen <= n_genoA) return true;
    }
    else { // male X
        if(gen > n_genoA && gen <= n_genoX) return true;
    }

    return false; // otherwise a problem
}

const double AIL3::init(const int true_gen,
                        const bool is_x_chr, const bool is_female,
                        const IntegerVector& cross_info)
{
    #ifndef RQTL2_NODEBUG
    if(!check_geno(true_gen, false, is_x_chr, is_female, cross_info))
        throw std::range_error("genotype value not allowed");
    #endif

    if(!is_x_chr || is_female) { // autosome or female X
        if(mpp_is_het(true_gen, 3, false)) return log(2.0)-log(9.0);
        else return log(1.0)-log(9.0);
    }
    else { // male X
        return -log(3.0);
    }
}

const double AIL3::emit(const int obs_gen, const int true_gen, const double error_prob,
                        const IntegerVector& founder_geno, const bool is_x_chr,
                        const bool is_female, const IntegerVector& cross_info)
{
    #ifndef RQTL2_NODEBUG
    if(!check_geno(true_gen, false, is_x_chr, is_female, cross_info))
        throw std::range_error("genotype value not allowed");
    #endif

    const int n_geno = 6;

    if(obs_gen==0) return 0.0; // missing

    if(!is_x_chr || is_female) { // autosome or female X
        const IntegerVector true_alleles = mpp_decode_geno(true_gen, 3, false);
        int f1 = founder_geno[true_alleles[0]-1];
        int f2 = founder_geno[true_alleles[1]-1];

        // treat founder hets as missing
        if(f1==2) f1 = 0;
        if(f2==2) f2 = 0;

        // neither founder alleles observed
        if(f1 == 0 && f2 == 0) return 0.0;

        // one founder allele observed
        if(f1 == 0 || f2 == 0) {

            switch(std::max(f1, f2)) {
            case H: return 0.0; // het compatible with either founder allele
            case A:
                switch(obs_gen) {
                case A: case notB: return log(1.0-error_prob);
                case B: case notA: return log(error_prob);
                case H: return 0.0;
                }
            case B:
                switch(obs_gen) {
                case B: case notA: return log(1.0-error_prob);
                case A: case notB: return log(error_prob);
                case H: return 0.0;
                }
            }
            return 0.0;
        }

        switch((f1+f2)/2) { // values 1, 2, 3
        case A:
            switch(obs_gen) {
            case A: return log(1.0-error_prob);
            case H: return log(error_prob/2.0);
            case B: return log(error_prob/2.0);
            case notA: return log(error_prob);
            case notB: return log(1.0-error_prob/2.0);
            }
        case H:
            switch(obs_gen) {
            case A: return log(error_prob/2.0);
            case H: return log(1.0-error_prob);
            case B: return log(error_prob/2.0);
            case notA: return log(1.0-error_prob/2.0);
            case notB: return log(1.0-error_prob/2.0);
            }
        case B:
            switch(obs_gen) {
            case B: return log(1.0-error_prob);
            case H: return log(error_prob/2.0);
            case A: return log(error_prob/2.0);
            case notB: return log(error_prob);
            case notA: return log(1.0-error_prob/2.0);
            }
        }
        return 0.0;
    }
    else { // male X
        const int founder_allele = founder_geno[(true_gen - n_geno) - 1];

        switch(founder_allele) {
        case A:
            switch(obs_gen) {
            case A: case notB: return log(1.0-error_prob);
            case B: case notA: return log(error_prob);
            }
        case B:
            switch(obs_gen) {
            case B: case notA: return log(1.0-error_prob);
            case A: case notB: return log(error_prob);
            }
        }
        return(0.0);
    }

    return NA_REAL; // shouldn't get here
}


const double AIL3::step(const int gen_left, const int gen_right, const double rec_frac,
                        const bool is_x_chr, const bool is_female,
                        const IntegerVector& cross_info)
{
    #ifndef RQTL2_NODEBUG
    if(!check_geno(gen_left, false, is_x_chr, is_female, cross_info) ||
       !check_geno(gen_right, false, is_x_chr, is_female, cross_info))
        throw std::range_error("genotype value not allowed");
    #endif

    const int n_gen = cross_info[0]; // number of generations

    // don't let rf get too small on X chr
    double rf = rec_frac;
    if(is_x_chr && rf < 1e-8) rf = 1e-8;

    if(is_x_chr && !is_female) { // male X
        const double z = sqrt((1.0-rf)*(9.0-rf));
        const double pAA = (1.0-rf)/3.0 * ( (1.0-rf+z)/(2.0*z) * pow((1.0-rf-z)/4.0, (double)(n_gen-2)) +
                                                  (-1.0+rf+z)/(2.0*z) * pow((1.0-rf+z)/4.0, (double)(n_gen-2))) +
            (2.0-rf)/(2.0*3) * ( (1.0-rf-z)/2.0 * (1.0-rf+z)/(2.0*z) * pow((1.0-rf-z)/4.0,(double)(n_gen-2)) +
                                       (1.0-rf+z)/2.0 * (-1.0+rf+z)/(2.0*z) * pow((1.0-rf+z)/4.0,(double)(n_gen-2))) +
            ( (rf*rf + rf*(z-5.0))/(9*(3.0+rf+z)) * (1.0-rf+z)/(2.0*z) * pow((1.0-rf-z)/4.0,(double)(n_gen-2)) +
              (rf*rf - rf*(z+5.0))/(9*(3.0+rf-z)) * (-1.0+rf+z)/(2.0*z) * pow((1.0-rf+z)/4.0,(double)(n_gen-2)) + 1.0/9.0);
        const double R = (1.0-3.0*pAA);

        if(gen_left == gen_right) return log1p(-R);
        return log(R) - log(2.0);
    }
    else { // autosome or female X
        double pAA;
        if(!is_x_chr) { // autosome
            pAA = (1.0 - (-2.0 + 3.0*rf)*pow(1.0 - rf, (double)(n_gen-2)))/9.0;
        }
        else { // female X
            const double z = sqrt((1.0-rf)*(9.0-rf));

            pAA = (1.0-rf)/3.0 * ( (-1.0/z)*pow((1.0-rf-z)/4.0,(double)(n_gen-2)) +
                                         (1.0/z)*pow((1.0-rf+z)/4.0,(double)(n_gen-2))) +
                (2.0-rf)/6.0 * ( (1.0-rf-z)/2.0 * (-1.0/z)*pow((1.0-rf-z)/4.0,(double)(n_gen-2)) +
                                       (1.0-rf+z)/2.0 *  (1.0/z)*pow((1.0-rf+z)/4.0,(double)(n_gen-2))) +
                ( (rf*rf + rf*(z-5.0))/(9.0*(3.0+rf+z)) * (-1.0/z)*pow((1.0-rf-z)/4.0,(double)(n_gen-2)) +
                  (rf*rf - rf*(z+5.0))/(9.0*(3.0+rf-z)) * (1.0/z)*pow((1.0-rf+z)/4.0,(double)(n_gen-2))  + 1.0/9.0);
        }
        double R = (1.0 - 3.0*pAA);

        const IntegerVector alleles_left = mpp_decode_geno(gen_left, 3, false);
        const IntegerVector alleles_right = mpp_decode_geno(gen_right, 3, false);

        if(alleles_left[0] == alleles_left[1]) { // homozygous
            if(alleles_right[0] == alleles_right[1]) { // homozygous
                if(alleles_left[0] == alleles_right[0]) { // AA -> AA
                    return 2.0*log(1.0-R);
                }
                else { // AA -> BB
                    return 2.0*(log(R) - log(2.0));
                }
            }
            else {
                if(alleles_left[0] == alleles_right[0] ||
                   alleles_left[0] == alleles_right[1]) { // AA -> AB
                    return log(1.0-R) + log(R) - log(2.0);
                }
                else { // AA -> BC
                    return 2.0*(log(R)-log(2.0));
                }
            }
        }
        else {
            if(alleles_right[0] == alleles_right[1]) { // homozygous
                if(alleles_left[0] == alleles_right[0] ||
                   alleles_left[1] == alleles_right[1]) { // AB -> AA
                    return log(1.0-R) + log(R) - log(2.0);
                }
                else { // AB -> CC
                    return 2.0*(log(R) - log(2.0));
                }
            }
            else {
                if((alleles_left[0] == alleles_right[0] &&
                    alleles_left[1] == alleles_right[1]) ||
                   (alleles_left[0] == alleles_right[1] &&
                    alleles_left[1] == alleles_right[0])) { // AB -> AB
                    return log((1.0-R)*(1.0-R) + R*R/4.0);
                }
                else { // AB -> BC  (R/2)^2 + (R/2)*(1-R) = (R/2)*(1-R/2)
                    return log(R) - log(2.0) + log(1.0 - R/2.0);
                }
            }
        }

    }

    return NA_REAL; // shouldn't get here
}

const IntegerVector AIL3::possible_gen(const bool is_x_chr, const bool is_female,
                                     const IntegerVector& cross_info)
{
    if(is_x_chr && !is_female) { // male X chromosome
        IntegerVector result = IntegerVector::create(7,8,9);
        return result;
    }
    else { // autosome or female X
        IntegerVector result = IntegerVector::create(1,2,3,4,5,6);
        return result;
    }
}

const int AIL3::ngen(const bool is_x_chr)
{
    if(is_x_chr) return 9;
    return 6;
}

const int AIL3::nalleles()
{
    return 3;
}

const NumericMatrix AIL3::geno2allele_matrix(const bool is_x_chr)
{
    if(is_x_chr) {
        NumericMatrix result(9,3);
        result(0,0) = 1.0;               // AA female
        result(1,0) = result(1,1) = 0.5; // AB female
        result(2,1) = 1.0;               // BB female
        result(3,0) = result(3,2) = 0.5; // AC female
        result(4,1) = result(4,2) = 0.5; // BC female
        result(5,2) = 1.0;               // CC female

        result(6,0) = 1.0; // AY male
        result(7,1) = 1.0; // BY male
        result(8,2) = 1.0; // CY male

        return result;
    }
    else {
        NumericMatrix result(6,3);
        result(0,0) = 1.0;               // AA
        result(1,0) = result(1,1) = 0.5; // AB
        result(2,1) = 1.0;               // BB
        result(3,0) = result(3,2) = 0.5; // AC
        result(4,1) = result(4,2) = 0.5; // BC
        result(5,2) = 1.0;               // CC

        return result;
    }
}

// check that sex conforms to expectation
const bool AIL3::check_is_female_vector(const LogicalVector& is_female, const bool any_x_chr)
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
const bool AIL3::check_crossinfo(const IntegerMatrix& cross_info, const bool any_x_chr)
{
    bool result = true;
    const int n_row = cross_info.rows();
    const int n_col = cross_info.cols();
    // single column with number of generations (needed no matter what; values should be >= 2)

    if(n_col != 1) {
        result = false;
        r_message("cross_info should have one column, with no. generations");
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
        r_message("cross_info has missing values (it shouldn't)");
    }
    if(n_invalid > 0) {
        result = false;
        r_message("cross_info has invalid values; no. generations should be >= 2");
    }

    return result;
}

// geno_names from allele names
const std::vector<std::string> AIL3::geno_names(const std::vector<std::string> alleles,
                                                const bool is_x_chr)
{
    if(alleles.size() != 3)
        throw std::range_error("alleles must have length 3");

    if(is_x_chr) {
        std::vector<std::string> result(9);
        result[0] = alleles[0] + alleles[0];
        result[1] = alleles[0] + alleles[1];
        result[2] = alleles[1] + alleles[1];
        result[3] = alleles[0] + alleles[2];
        result[4] = alleles[1] + alleles[2];
        result[5] = alleles[2] + alleles[2];
        result[6] = alleles[0] + "Y";
        result[7] = alleles[1] + "Y";
        result[8] = alleles[2] + "Y";
        return result;
    }
    else {
        std::vector<std::string> result(6);
        result[0] = alleles[0] + alleles[0];
        result[1] = alleles[0] + alleles[1];
        result[2] = alleles[1] + alleles[1];
        result[3] = alleles[0] + alleles[2];
        result[4] = alleles[1] + alleles[2];
        result[5] = alleles[2] + alleles[2];
        return result;
    }
}

// used in counting crossovers in count_xo.R and locate_xo.R
const int AIL3::nrec(const int gen_left, const int gen_right,
                     const bool is_x_chr, const bool is_female,
                     const Rcpp::IntegerVector& cross_info)
{
    #ifndef RQTL2_NODEBUG
    if(!check_geno(gen_left, false, is_x_chr, is_female, cross_info) ||
       !check_geno(gen_right, false, is_x_chr, is_female, cross_info))
        throw std::range_error("genotype value not allowed");
    #endif

    if(is_x_chr && gen_left > 6 && gen_right > 6) { // male X
        if(gen_left == gen_right) return(0);
        else return(1);
    }

    // otherwise autosome or female X
    Rcpp::IntegerVector a_left = mpp_decode_geno(gen_left, 3, false);
    Rcpp::IntegerVector a_right = mpp_decode_geno(gen_right, 3, false);

    if(a_left[0] == a_right[0]) {
        if(a_left[1] == a_right[1]) return(0);
        else return(1);
    }
    else if(a_left[0] == a_right[1]) {
        if(a_left[1] == a_right[0]) return(0);
        else return(1);
    }
    else if(a_left[1] == a_right[0]) {
        return(1);
    }
    else if(a_left[1] == a_right[1]) {
        return(1);
    }
    else return(2);

    return(NA_INTEGER);
}

// not implemented
const NumericVector AIL3::est_map2(const IntegerMatrix& genotypes,
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
    Rcpp::stop("est_map not yet implemented for 3-way AILs.");

    // return vector of NAs
    const int n_rf = rec_frac.size();
    NumericVector result(n_rf);
    for(int i=0; i<n_rf; i++) result[i] = NA_REAL;
    return result ;
}

// check that founder genotype data has correct no. founders and markers
const bool AIL3::check_founder_geno_size(const IntegerMatrix& founder_geno, const int n_markers)
{
    bool result=true;

    const int fg_mar = founder_geno.cols();
    const int fg_f   = founder_geno.rows();

    if(fg_mar != n_markers) {
        result = false;
        r_message("founder_geno has incorrect number of markers");
    }

    if(fg_f != 3) {
        result = false;
        r_message("founder_geno should have 3 founders");
    }

    return result;
}

// check that founder genotype data has correct values
const bool AIL3::check_founder_geno_values(const IntegerMatrix& founder_geno)
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

const bool AIL3::need_founder_geno()
{
    return true;
}
