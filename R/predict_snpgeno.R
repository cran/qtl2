#' Predict SNP genotypes
#'
#' Predict SNP genotypes in a multiparent population from inferred genotypes plus founder strains' SNP alleles.
#'
#' @param cross Object of class `"cross2"`. For details, see the
#' [R/qtl2 developer guide](https://kbroman.org/qtl2/assets/vignettes/developer_guide.html).
#' @param geno Imputed genotypes, as a list of matrices, as from [maxmarg()].
#' @param cores Number of CPU cores to use, for parallel calculations.
#' (If `0`, use [parallel::detectCores()].)
#' Alternatively, this can be links to a set of cluster sockets, as
#' produced by [parallel::makeCluster()].
#'
#' @return
#' A list of matrices with inferred SNP genotypes, coded 1/2/3.
#'
#' @keywords utilities
#' @seealso [maxmarg()], [viterbi()], [calc_errorlod()]
#' @export
#' @importFrom stats setNames
#'
#' @examples
#' \dontrun{
#' # load example data and calculate genotype probabilities
#' file <- paste0("https://raw.githubusercontent.com/rqtl/",
#'                "qtl2data/main/DOex/DOex.zip")
#' DOex <- read_cross2(file)
#' probs <- calc_genoprob(DOex, error_prob=0.002)
#'
#' # inferred genotypes
#' m <- maxmarg(probs, minprob=0.5)
#'
#' # inferred SNP genotypes
#' inferg <- predict_snpgeno(DOex, m)
#' }
#'
predict_snpgeno <-
function(cross, geno, cores=1)
{
   if(!("founder_geno" %in% names(cross))) {
      stop("cross doesn't contain founder genotypes")
   }

   # ensure same chromosomes
   if(n_chr(cross) != length(geno) ||
      any(chr_names(cross) != names(geno))) {
       chr <- get_common_ids(setNames(names(geno), NULL), setNames(chr_names(cross), NULL))
       cross <- cross[,chr]
       geno <- geno[,chr]
   }

   # ensure same individuals
   if(n_ind_geno(cross) != nrow(geno[[1]]) ||
      any(ind_ids_geno(cross) != rownames(geno[[1]]))) {
       ind <- get_common_ids(setNames(rownames(geno[[1]]), NULL), setNames(ind_ids_geno(cross), NULL))
       cross <- cross[ind,]
       geno <- geno[ind,]
   }

   phase_known <- is_phase_known(cross)

   # guess phase
   if(!phase_known) {
       ph <- guess_phase(cross, geno, cores=cores)
   } else {
       ph <- cross$geno
   }

   # subset cross chromosomes
   cross <- cross[,names(ph)]

   # X chr and sexes
   is_x_chr <- cross$is_x_chr
   is_male <- !cross$is_female

   # set up cluster; use quiet=TRUE
   cores <- setup_cluster(cores, TRUE)

   by_chr_func <- function(chr) {
       # ensure the same markers
       fg <- cross$founder_geno[[chr]]
       if(phase_known) {
           ph1 <- ph2 <- geno[[chr]]
       } else {
           ph1 <- ph[[chr]][,,1]
           ph2 <- ph[[chr]][,,2]
       }

       # ugh deal with DOF1/HSF1 specially
       if(cross$crosstype %in% c("dof1", "hsf1")) {
           if(!is_x_chr[chr]) ph2[] <- 9 # one chromosome is from 9th founder
           else {
               if(any(is_male)) ph2[is_male,] <- ph1[is_male,,drop=FALSE]
               if(any(!is_male)) ph2[!is_male,] <- 9
           }
       }

       if(ncol(fg) != ncol(ph1) ||
          any(colnames(fg) != colnames(ph1))) {
           mar <- get_common_ids(setNames(colnames(fg), NULL), setNames(colnames(ph1), NULL))
           fg <- fg[,mar,drop=FALSE]
           ph1 <- ph1[,mar,drop=FALSE]
           ph2 <- ph2[,mar,drop=FALSE]
       }

       result <- .predict_snpgeno(ph1, ph2, fg)
       dimnames(result) <- dimnames(cross$geno[[chr]])

       if(is_x_chr[chr] && any(is_male)) { # some males on X chr
           result[is_male,] <- .predict_snpgeno(ph1[is_male,,drop=FALSE], ph1[is_male,,drop=FALSE], fg)
       }

       result
   }

   result <- cluster_lapply(cores, seq_along(geno), by_chr_func)
   names(result) <- names(geno)
   result

}
