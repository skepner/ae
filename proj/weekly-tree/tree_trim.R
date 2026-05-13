#! /usr/bin/env Rscript

main = function() {
  message("TRIM TREE")

  .libPaths(
    c(
      "/home/samt123/R/x86_64-pc-linux-gnu-library/4.4",
      .libPaths()
    )
  )

  library(seqUtils, quietly = T, warn.conflicts = F, verbose = F)
  library(convergence, quietly = T, warn.conflicts = F, verbose = F)
  library(treeio, quietly = T, warn.conflicts = F, verbose = F, )
  library(purrr, quietly = T, warn.conflicts = F, verbose = F)
  library(dplyr, quietly = T, warn.conflicts = F, verbose = F)
  library(stringr, quietly = T, warn.conflicts = F, verbose = F)

  convergence:::add_to_PATH("/home/samt123/miniforge3/envs/treebuild/bin") # usher

  # args ------------------------------------------------------------
  args <- commandArgs(trailingOnly = TRUE)

  subtype = args[[1]]
  inphy.txt = args[[2]]
  infasta = args[[3]]

  inphy = readLines(inphy.txt, n = 1)

  outphy = paste0(
    paste0(
      fs::path_ext_remove(inphy),
      "-trim."
    ),
    fs::path_ext(inphy)
  )

  outphy.txt = paste0(
    paste0(
      fs::path_ext_remove(inphy.txt),
      "-trim."
    ),
    fs::path_ext(inphy.txt)
  )

  outfasta = paste0(
    paste0(
      fs::path_ext_remove(infasta),
      "-trim."
    ),
    fs::path_ext(infasta)
  )

  if (all(fs::file_exists(c(outphy, outphy.txt, outfasta)))) {
    message("All output files already exist. Not remaking.")
  }

  # specify trim ----------------------------------------------------
  if (subtype == "h3") {
    trim = T
    mutation = "223V"
    required_mutations = list() # list(list(at = 100, to = "T"), list(at = ...))
    forbidden_mutations = list()
    min_tips = 15000
  } else if (subtype == "h1") {
    trim = F
    mutation = NA
    required_mutations = list()
    forbidden_mutations = list()
    min_tips = 0
  } else if (subtype == "bvic") {
    trim = T
    mutation = "127T"
    required_mutations = list()
    forbidden_mutations = list()
    min_tips = 15000
  } else if (subtype == "byam") {
    trim = F
    mutation = NA
    required_mutations = list()
    forbidden_mutations = list()
    min_tips = 0
  } else {
    stop("Invalid subtype ", subtype)
  }

  # read input files ------------------------------------------------------------

  phy = ape::read.tree(file = inphy)
  fa = seqUtils::fast_fasta(infasta)

  # trim ------------------------------------------------------------
  if (!trim) {
    trimmed_phy = phy
    trimmed_seqs = fa
  } else {
    tree_and_sequences = convergence::makeTreeAndSequences(
      tree = phy,
      sequences = tibble::tibble(
        Isolate_unique_identifier = names(fa),
        dna_sequence = seqUtils::clean_sequences(unname(fa), type = "nt")
      )
    )

    usher_tree_and_sequences = convergence::addASRusher(
      tree_and_sequences,
      nuc_ref = fa[1],
      aa_ref = as.character(Biostrings::translate(Biostrings::DNAString(fa[1])))
    )

    usher_tree_and_sequences$tree_tibble$nd = c(
      rep(1, ape::Ntip(usher_tree_and_sequences$tree)),
      castor::count_tips_per_node(usher_tree_and_sequences$tree)
    )

    tree_tibble_mutation_occs = usher_tree_and_sequences$tree_tibble %>%
      filter(
        map_lgl(aa_mutations_nonsyn, ~ mutation %in% str_sub(.x, 2)),
        nd >= min_tips
      ) %>%
      arrange(-nd)

    for (r_m in required_mutations) {
      at = as.integer(r_m[["at"]])
      to = r_m[["to"]]
      tree_tibble_mutation_occs = tree_tibble_mutation_occs %>%
        filter(substr(reconstructed_aa_sequence, at, at) == to)
    }

    for (f_m in required_mutations) {
      at = as.integer(f_m[["at"]])
      to = f_m[["to"]]
      tree_tibble_mutation_occs = tree_tibble_mutation_occs %>%
        filter(substr(reconstructed_aa_sequence, at, at) != to)
    }

    if (nrow(tree_tibble_mutation_occs) != 1) {
      stop("No tree branches fulfilling specification found ")
    }

    ancestor = tree_tibble_mutation_occs$node[[1]]

    trimmed_phy = castor::get_subtree_at_node(
      usher_tree_and_sequences$tree,
      ancestor - ape::Ntip(usher_tree_and_sequences$tree)
    )$subtree

    trimmed_phy = ape::ladderize(trimmed_phy, right = F)
    trimmed_phy$edge.length = trimmed_phy$edge.length / mean(nchar(fa)) # per nt
    trimmed_seqs = fa[names(fa) %in% trimmed_phy$tip.label]
  }

  message("Writing tree")
  ape::write.tree(trimmed_phy, file = outphy)

  message("Writing ", outphy.txt)
  writeLines(outphy, outphy.txt)

  message("Writing ", outfasta)
  seqUtils::write_fast_fasta(
    seqs = unname(trimmed_seqs),
    names = names(trimmed_seqs),
    path = outfasta
  )
  return(0)
}

main()
