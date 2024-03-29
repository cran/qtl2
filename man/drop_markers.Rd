% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/drop_markers.R
\name{drop_markers}
\alias{drop_markers}
\title{Drop markers from a cross2 object}
\usage{
drop_markers(cross, markers)
}
\arguments{
\item{cross}{Object of class \code{"cross2"}. For details, see the
\href{https://kbroman.org/qtl2/assets/vignettes/developer_guide.html}{R/qtl2 developer guide}.}

\item{markers}{A vector of marker names.}
}
\value{
The input \code{cross} with the specified markers removed.
}
\description{
Drop a vector of markers from a cross2 object.
}
\examples{
grav2 <- read_cross2(system.file("extdata", "grav2.zip", package="qtl2"))
markers2drop <- c("BH.342C/347L-Col", "GH.94L", "EG.357C/359L-Col", "CD.245L", "ANL2")
grav2_rev <- drop_markers(grav2, markers2drop)
}
\seealso{
\code{\link[=pull_markers]{pull_markers()}}, \code{\link[=drop_nullmarkers]{drop_nullmarkers()}}, \code{\link[=reduce_markers]{reduce_markers()}}, \code{\link[=find_dup_markers]{find_dup_markers()}}
}
