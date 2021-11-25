def hamming_distance(seq1: str, seq2: str) -> int:
    dist = 0
    for i, nuc in enumerate(seq1):
        if nuc != seq2[i]:
            dist += 1
    return dist

# ======================================================================
