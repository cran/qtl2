// Converting genotype/allele probabilities to SNP probs

#include "snpprobs.h"
#include <exception>
#include <Rcpp.h>
using namespace Rcpp;


// calculate strain distribution pattern (SDP) from
// SNP genotypes for a set of strains
//
// Input is a marker x strain matrix of genotypes
// 0 = homozygous AA, 1 = homozygous BB
//
// [[Rcpp::export(".calc_sdp")]]
IntegerVector calc_sdp(const IntegerMatrix& geno)
{
    const int n_mar = geno.rows();
    const int n_str = geno.cols();
    if(n_str < 2)
        throw std::invalid_argument("Need genotypes on >= 2 strains");

    IntegerVector result(n_mar);
    for(int i=0; i<n_mar; i++) {
        for(int j=0; j<n_str; j++) {
            result[i] += geno(i,j)*(1 << j);
        }
    }

    return result;
}

// calculate SNP genotypes from a set of
// strain distribution patterns (SDPs)
//
// Input is a vector of SDPs + number of strains
//
// Output is a marker x strain matrix of genotypes
// 0 = homozygous AA, 1 = homozygous BB
//
// [[Rcpp::export(".invert_sdp")]]
IntegerMatrix invert_sdp(const IntegerVector& sdp, const int n_str)
{
    const int n_mar = sdp.size();
    for(int mar=0; mar<n_mar; mar++) {
        if(sdp[mar] < 0 || sdp[mar] > (1 << n_str) - 1)
            throw std::invalid_argument("sdp out of range");
    }

    IntegerMatrix result(n_mar,n_str);

    for(int mar=0; mar<n_mar; mar++) {
        for(int str=0; str<n_str; str++) {
            result(mar,str) = (sdp[mar] & (1 << str)) != 0;
        }
    }

    return result;
}

// convert allele probabilities into SNP probabilities
//
// alleleprob = individual x allele x position
// sdp = vector of strain distribution patterns
// interval = map interval containing snp
// on_map = logical vector indicating snp is at left endpoint of interval
//
// [[Rcpp::export(".alleleprob_to_snpprob")]]
NumericVector alleleprob_to_snpprob(const NumericVector& alleleprob,
                                    const IntegerVector& sdp,
                                    const IntegerVector& interval,
                                    const LogicalVector& on_map)
{
    if(Rf_isNull(alleleprob.attr("dim")))
        throw std::invalid_argument("alleleprob should be a 3d array but has no dim attribute");
    const IntegerVector& d = alleleprob.attr("dim");
    if(d.size() != 3)
        throw std::invalid_argument("alleleprob should be a 3d array");
    const int n_ind = d[0];
    const int n_str = d[1];
    const int n_pos = d[2];
    const int n_snp = sdp.size();
    if(n_snp != interval.size())
        throw std::invalid_argument("length(sdp) != length(interval)");
    if(n_snp != on_map.size())
        throw std::invalid_argument("length(sdp) != length(on_map)");
    if(n_str < 3) // not meaningful for <3 strains
        throw std::invalid_argument("meaningful only with >= 3 strains");

    NumericVector result(n_ind*2*n_snp);
    result.attr("dim") = Dimension(n_ind, 2, n_snp);

    // check that the interval and SDP values are okay
    for(int i=0; i<n_snp; i++) {
        if(interval[i] < 0 || interval[i] > n_pos-1 ||
           (interval[i] == n_pos-1 && !on_map[i]))
            throw std::invalid_argument("snp outside of map range");
        if(sdp[i] < 1 || sdp[i] > (1 << n_str)-1)
            throw std::invalid_argument("SDP out of range");
    }

    for(int snp=0; snp<n_snp; snp++) {
        for(int strain=0; strain < n_str; strain++) {

            int allele = ((sdp[snp] & (1 << strain)) != 0); // 0 or 1 SNP genotype for this strain

            int result_offset = allele*n_ind + (snp*n_ind*2);
            int input_offset = strain*n_ind + (interval[snp]*n_ind*n_str);
            int next_on_map = input_offset + n_ind*n_str;
            for(int ind=0; ind<n_ind; ind++) {
                if(on_map[snp]) {
                    result[ind + result_offset] += alleleprob[ind + input_offset];
                }
                else {
                    result[ind + result_offset] += (alleleprob[ind + input_offset] +
                                                    alleleprob[ind + next_on_map])/2.0;
                }
            } // loop over individuals
        }
    } // loop over snps

    return result;
}

// convert genotype columns to SNP columns
//
// n_str     Number of strains
//    (so n_str*(n_str+1)/2 columns)
// sdp       Strain distribution pattern for SNP
//
// [[Rcpp::export]]
IntegerVector genocol_to_snpcol(const int n_str, const int sdp)
{
    const int n_gen = n_str*(n_str+1)/2;
    if(sdp < 1 || sdp > (1 << n_str)-1)
        throw std::invalid_argument("SDP out of range");

    IntegerVector result(n_gen);

    for(int a1=0, g=0; a1<n_str; a1++) {
        for(int a2=0; a2<=a1; a2++, g++) {
            // a1,a2 are the alleles
            // g is the corresponding genotype code

            // snp alleles for these two alleles
            int snp1 = ((sdp & (1 << a1)) != 0);
            int snp2 = ((sdp & (1 << a2)) != 0);

            if(snp1==0 && snp2==0) { // hom 00
                result[g] = 0;
            }
            else if(snp1==1 && snp2==1) { // hom 11
                result[g] = 2;
            }
            else { // het
                result[g] = 1;
            }
        }
    }

    return result;
}

// convert genotype probabilities into SNP probabilities
//
// genoprob = individual x genotype x position
// sdp = vector of strain distribution patterns
// interval = map interval containing snp
// on_map = logical vector indicating snp is at left endpoint of interval
//
// [[Rcpp::export(".genoprob_to_snpprob")]]
NumericVector genoprob_to_snpprob(const NumericVector& genoprob,
                                  const IntegerVector& sdp,
                                  const IntegerVector& interval,
                                  const LogicalVector& on_map)
{
    if(Rf_isNull(genoprob.attr("dim")))
        throw std::invalid_argument("genoprob should be a 3d array but has no dim attribute");
    const IntegerVector& d = genoprob.attr("dim");
    if(d.size() != 3)
        throw std::invalid_argument("genoprob should be a 3d array");
    const int n_ind = d[0];
    const int n_gen = d[1];
    const int n_str = (sqrt(8*n_gen + 1) - 1)/2;
    if(n_gen != n_str*(n_str+1)/2)
        throw std::invalid_argument("n_gen must == n(n+1)/2 for some n");
    const int n_pos = d[2];
    const int n_snp = sdp.size();
    if(n_snp != interval.size())
        throw std::invalid_argument("length(sdp) != length(interval)");
    if(n_snp != on_map.size())
        throw std::invalid_argument("length(sdp) != length(on_map)");
    if(n_str < 3) // not meaningful for <3 strains
        throw std::invalid_argument("meaningful only with >= 3 strains");

    NumericVector result(n_ind*3*n_snp); // 3 is the number of SNP genotypes (AA,AB,BB)
    result.attr("dim") = Dimension(n_ind, 3, n_snp);

    // check that the interval and SDP values are okay
    for(int i=0; i<n_snp; i++) {
        if(interval[i] < 0 || interval[i] > n_pos-1 ||
           (interval[i] == n_pos-1 && !on_map[i]))
            throw std::invalid_argument("snp outside of map range");
        if(sdp[i] < 1 || sdp[i] > (1 << n_str)-1)
            throw std::invalid_argument("SDP out of range");
    }

    for(int snp=0; snp<n_snp; snp++) {
        IntegerVector snpcol = genocol_to_snpcol(n_str, sdp[snp]);

        for(int g=0; g<n_gen; g++) {
            int result_offset = snpcol[g]*n_ind + (snp*n_ind*3);
            int input_offset = g*n_ind + (interval[snp]*n_ind*n_gen);
            int next_on_map = input_offset + n_ind*n_gen;
            for(int ind=0; ind<n_ind; ind++) {
                if(on_map[snp]) {
                    result[ind + result_offset] += genoprob[ind + input_offset];
                }
                else {
                    result[ind + result_offset] += (genoprob[ind + input_offset] +
                                                    genoprob[ind + next_on_map])/2.0;
                }
            } // loop over individuals
        }
    } // loop over snps

    return result;
}

// convert X genotype columns to SNP columns
//
// n_str     Number of strains
//    (so n_str + n_str*(n_str+1)/2 columns)
// sdp       Strain distribution pattern for SNP
//
// [[Rcpp::export]]
IntegerVector Xgenocol_to_snpcol(const int n_str, const int sdp)
{
    const int n_femgen = n_str*(n_str+1)/2;
    const int n_gen = n_femgen + n_str;
    if(sdp < 1 || sdp > (1 << n_str)-1)
        throw std::invalid_argument("SDP out of range");

    IntegerVector result(n_gen);

    for(int a1=0, g=0; a1<n_str; a1++) {
        int snp1;
        for(int a2=0; a2<=a1; a2++, g++) {
            // a1,a2 are the alleles
            // g is the corresponding genotype code

            // snp alleles for these two alleles
            snp1 = ((sdp & (1 << a1)) != 0);
            int snp2 = ((sdp & (1 << a2)) != 0);

            if(snp1==0 && snp2==0) { // hom 00
                result[g] = 0;
            }
            else if(snp1==1 && snp2==1) { // hom 11
                result[g] = 2;
            }
            else { // het
                result[g] = 1;
            }
        }

        // male hemizgygotes
        result[n_femgen + a1] = 3 + snp1;
    }

    return result;
}

// convert X chr genotype probabilities into SNP probabilities
//
// here the genotypes are the 36 female genotypes followed by the 8 male genotypes
//
// genoprob = individual x genotype x position
// sdp = vector of strain distribution patterns
// interval = map interval containing snp
// on_map = logical vector indicating snp is at left endpoint of interval
//
// [[Rcpp::export(".Xgenoprob_to_snpprob")]]
NumericVector Xgenoprob_to_snpprob(const NumericVector& genoprob,
                                   const IntegerVector& sdp,
                                   const IntegerVector& interval,
                                   const LogicalVector& on_map)
{
    if(Rf_isNull(genoprob.attr("dim")))
        throw std::invalid_argument("genoprob should be a 3d array but has no dim attribute");
    const IntegerVector& d = genoprob.attr("dim");
    if(d.size() != 3)
        throw std::invalid_argument("genoprob should be a 3d array");
    const int n_ind = d[0];
    const int n_gen = d[1];
    const int n_str = (sqrt(8*n_gen + 9) - 3)/2;
    if(n_gen != n_str*(n_str+1)/2 + n_str)
        throw std::invalid_argument("n_gen must == n + n(n+1)/2 for some n");
    const int n_pos = d[2];
    const int n_snp = sdp.size();
    if(n_snp != interval.size())
        throw std::invalid_argument("length(sdp) != length(interval)");
    if(n_snp != on_map.size())
        throw std::invalid_argument("length(sdp) != length(on_map)");
    if(n_str < 3) // not meaningful for <3 strains
        throw std::invalid_argument("meaningful only with >= 3 strains");

    NumericVector result(n_ind*5*n_snp); // 5 is the number of SNP genotypes (AA,AB,BB,AY,BY)
    result.attr("dim") = Dimension(n_ind, 5, n_snp);

    // check that the interval and SDP values are okay
    for(int i=0; i<n_snp; i++) {
        if(interval[i] < 0 || interval[i] > n_pos-1 ||
           (interval[i] == n_pos-1 && !on_map[i]))
            throw std::invalid_argument("snp outside of map range");
        if(sdp[i] < 1 || sdp[i] > (1 << n_str)-1)
            throw std::invalid_argument("SDP out of range");
    }

    for(int snp=0; snp<n_snp; snp++) {
        IntegerVector snpcol = Xgenocol_to_snpcol(n_str, sdp[snp]);

        for(int g=0; g<n_gen; g++) {
            int result_offset = snpcol[g]*n_ind + (snp*n_ind*5);
            int input_offset = g*n_ind + (interval[snp]*n_ind*n_gen);
            int next_on_map = input_offset + n_ind*n_gen;
            for(int ind=0; ind<n_ind; ind++) {
                if(on_map[snp]) {
                    result[ind + result_offset] += genoprob[ind + input_offset];
                }
                else {
                    result[ind + result_offset] += (genoprob[ind + input_offset] +
                                                    genoprob[ind + next_on_map])/2.0;
                }
            } // loop over individuals
        }
    } // loop over snps

    return result;
}
