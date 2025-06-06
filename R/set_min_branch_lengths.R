#! /usr/bin/env Rscript

args <- commandArgs(trailingOnly = TRUE)

infile = args[[1]]
outfile = args[[2]]
min_bl = as.numeric(args[[3]])

t = ape::read.tree(file = infile)

t$edge.length[t$edge.length < min_bl] = min_bl

ape::write.tree(phy = t, file = outfile)
