#! /usr/bin/env Rscript
main = function() {
    message("ROTATE TREE")

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

    if (fs::file_exists(outphy) & fs::file_exists(outphy.txt)) {
        message(
            outphy,
            " and ",
            outphy.txt,
            " already exist! Continuing without re-rotating tree."
        )
        return(0)
    }

    message("Reading fasta")
    fa = ape::read.dna(
        file = infasta,
        format = "fasta",
        as.character = T,
        as.matrix = F
    )

    outgroup = names(fa)[[1]]

    message("Reading ", inphy)
    t = ape::read.tree(file = inphy)

    message("Rooting tree at ", outgroup)
    t = ape::root(t, outgroup = outgroup)
    message("Ladderizing tree")
    t = ape::ladderize(t, right = F)

    message("Writing tree")
    ape::write.tree(t, file = outphy)
    message("Writing ", outphy.txt)
    writeLines(outphy, outphy.txt)
    return(0)
}

main()
