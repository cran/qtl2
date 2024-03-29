% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/create_variant_query_func.R
\name{create_variant_query_func}
\alias{create_variant_query_func}
\title{Create a function to query variants}
\usage{
create_variant_query_func(
  dbfile = NULL,
  db = NULL,
  table_name = "variants",
  chr_field = "chr",
  pos_field = "pos",
  id_field = "snp_id",
  sdp_field = "sdp",
  filter = NULL
)
}
\arguments{
\item{dbfile}{Name of database file}

\item{db}{Optional database connection (provide one of \code{file} and \code{db}).}

\item{table_name}{Name of table in the database}

\item{chr_field}{Name of chromosome field}

\item{pos_field}{Name of position field}

\item{id_field}{Name of SNP/variant ID field}

\item{sdp_field}{Name of strain distribution pattern (SDP) field}

\item{filter}{Additional SQL filter (as a character string)}
}
\value{
Function with three arguments, \code{chr}, \code{start},
and \code{end}, which returns a data frame with the variants in
that region, with \code{start} and \code{end} being in Mbp. The
output should contain at least the columns \code{chr} and
\code{pos}, the latter being position in Mbp.
}
\description{
Create a function that will connect to a SQLite database of founder
variant information and return a data frame with variants for a
selected region.
}
\details{
Note that this function assumes that the database has a
\code{pos} field that is in basepairs, but the selection uses
\code{start} and \code{end} positions in Mbp, and the output
data frame should have \code{pos} in Mbp.

Also note that a SQLite database of variants in the founder strains
of the mouse Collaborative Cross is available at figshare:
\doi{10.6084/m9.figshare.5280229.v3}
}
\examples{
# create query function by connecting to file
dbfile <- system.file("extdata", "cc_variants_small.sqlite", package="qtl2")
query_variants <- create_variant_query_func(dbfile)
# query_variants will connect and disconnect each time
variants <- query_variants("2", 97.0, 98.0)

# create query function to just grab SNPs
query_snps <- create_variant_query_func(dbfile, filter="type=='snp'")
# query_variants will connect and disconnect each time
snps <- query_snps("2", 97.0, 98.0)

# connect and disconnect separately
library(RSQLite)
db <- dbConnect(SQLite(), dbfile)
query_variants <- create_variant_query_func(db=db)
variants <- query_variants("2", 97.0, 98.0)
dbDisconnect(db)
}
