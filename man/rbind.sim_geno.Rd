% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/rbind_sim_geno.R
\name{rbind.sim_geno}
\alias{rbind.sim_geno}
\title{Join genotype imputations for different individuals}
\usage{
\method{rbind}{sim_geno}(...)
}
\arguments{
\item{...}{Genotype imputation objects as produced by
\code{\link[=sim_geno]{sim_geno()}}. Must have the same set of markers and
genotypes.}
}
\value{
An object of class \code{"sim_geno"}, like the input; see \code{\link[=sim_geno]{sim_geno()}}.
}
\description{
Join multiple genotype imputation objects, as produced by
\code{\link[=sim_geno]{sim_geno()}}, for the same set of markers but for different
individuals.
}
\examples{
grav2 <- read_cross2(system.file("extdata", "grav2.zip", package="qtl2"))
map <- insert_pseudomarkers(grav2$gmap, step=1)
drawsA <- sim_geno(grav2[1:5,], map, error_prob=0.002, n_draws=4)
drawsB <- sim_geno(grav2[6:12,], map, error_prob=0.002, n_draws=4)
draws <- rbind(drawsA, drawsB)

}
\seealso{
\code{\link[=cbind.sim_geno]{cbind.sim_geno()}}, \code{\link[=sim_geno]{sim_geno()}}
}
