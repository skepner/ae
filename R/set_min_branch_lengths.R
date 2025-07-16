#! /usr/bin/env Rscript

main = function() {
  message("SET MINIMUM BRANCH LENGTHS")

  args <- commandArgs(trailingOnly = TRUE)

  infile = args[[1]]
  outfile = args[[2]]
  min_bl = as.numeric(args[[3]])

  if (fs::file_exists(outfile)) {
    message(outfile, " already exists. Not remaking.")
  }

  message("Reading tree")
  t = ape::read.tree(file = infile)

  message("Setting minimum branch lengths to ", min_bl)
  t$edge.length[t$edge.length < min_bl] = min_bl

  message("Writing tree")
  ape::write.tree(phy = t, file = outfile)
}

main()
