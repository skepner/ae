#! /usr/bin/env Rscript

args <- commandArgs(trailingOnly = TRUE)

inphy.txt = args[[1]]
tol = as.numeric(args[[2]])

# file names -------------------------------------------------------------
inphy = readLines(inphy.txt, n = 1)

outphy = paste0(
    paste0(
        fs::path_ext_remove(inphy),
        "-col."
    ),
    fs::path_ext(inphy)
)

outphy.txt = paste0(
    paste0(
        fs::path_ext_remove(inphy.txt),
        "-col."
    ),
    fs::path_ext(inphy.txt)
)

# collapse ---------------------------------------------------------------
phy = ape::read.tree(file = inphy)


# message(length(phy$edge.length))
# message(min(phy$edge.length))
# message(tol)
# message(sum(phy$edge.length < tol))

phy_col = ape::di2multi(phy, tol = tol)
message(ape::Nnode(phy), " -> ", ape::Nnode(phy_col))

ape::write.tree(phy_col, file = outphy)
writeLines(outphy, outphy.txt)
