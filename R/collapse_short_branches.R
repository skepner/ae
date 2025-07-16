#! /usr/bin/env Rscript
main = function() {
    message("COLLAPSE SHORT BRANCH LENGTHS")

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

    if (fs::file_exists(outphy) & fs::file_exists(outphy.txt)) {
        message(
            outphy,
            " and ",
            outphy.txt,
            " already exist! Continuing without re-rotating tree."
        )
        return(0)
    }

    message("Reading tree")
    phy = ape::read.tree(file = inphy)

    message("Collapsing branches shorter than ", tol)
    phy_col = ape::di2multi(phy, tol = tol)
    message(ape::Nnode(phy), " -> ", ape::Nnode(phy_col))

    message("Writing tree")
    ape::write.tree(phy_col, file = outphy)
    message("Writing ", outphy.txt)
    writeLines(outphy, outphy.txt)
}

main()
