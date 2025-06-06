#! /usr/bin/env Rscript

args <- commandArgs(trailingOnly = TRUE)

inphy.txt = args[[1]]
infasta = args[[2]]

# file names
inphy = readLines(inphy.txt, n = 1)

outphy = paste0(
    paste0(
        fs::path_ext_remove(inphy),
        "-rot."
    ),
    fs::path_ext(inphy)
)

outphy.txt = paste0(
    paste0(
        fs::path_ext_remove(inphy.txt),
        "-rot."
    ),
    fs::path_ext(inphy.txt)
)

# rotate
fa = ape::read.dna(
    file = infasta,
    format = "fasta",
    as.character = T,
    as.matrix = F
)

outgroup = names(fa)[[1]]

t = ape::read.tree(file = inphy)

t = ape::root(t, outgroup = outgroup)
t = ape::ladderize(t, right = F)

ape::write.tree(t, file = outphy)
writeLines(outphy, outphy.txt)
