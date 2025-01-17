#include "trim.h"

cutsites* Abstract_Trimmer::sliding_window(FQEntry &fqrec){
    //std::cout << "Starting sliding window\n";
	if(fqrec.seq.length() == 0){
		std::cout << "Sequence is empty!\n";
	}
	int window_size = (int) (0.1 * fqrec.seq.length());
	//std::cout << "Window size is: " << window_size << "\n";
	int i,j;
	int window_start=0;
	int window_total=0;
	int three_prime_cut = fqrec.seq.length();
	int five_prime_cut = 0;
	int found_five_prime = 0;
	double window_avg;
	cutsites* retvals;
    size_t npos;

	/* discard if the length of the sequence is less than the length threshold */
    if (fqrec.seq.length() < (size_t)length_threshold) {
		retvals = (cutsites*) malloc (sizeof(cutsites));
		retvals->three_prime_cut = -1;
		retvals->five_prime_cut = -1;
		return (retvals);
	}

	/* if the seq length is less then 10bp, */
	/* then make the window size the length of the seq */
	if (window_size == 0) window_size = fqrec.seq.length();
	for (i=0; i<window_size; i++) {
		window_total += get_quality_num (fqrec.qual.at(i), fqrec, i);
	}
	for (i=0; (size_t)i <= fqrec.qual.length() - (size_t)window_size; i++) {

		window_avg = (double)window_total / (double)window_size;

        if (debug) printf ("no_fiveprime: %d, found 5prime: %d, window_avg: %f\n", no_fiveprime, found_five_prime, window_avg);

		/* Finding the 5' cutoff */
		/* Find when the average quality in the window goes above the threshold starting from the 5' end */
		if (no_fiveprime == 0 && found_five_prime == 0 && window_avg >= qual_threshold) {
        	if (debug) printf ("inside 5-prime cut\n");

			/* at what point in the window does the quality go above the threshold? */
			for (j=window_start; j<window_start+window_size; j++) {
				if (get_quality_num (fqrec.qual.at(j), fqrec, j) >= qual_threshold) {
					five_prime_cut = j;
					break;
				}
			}

            if (debug) printf ("five_prime_cut: %d\n", five_prime_cut);

			found_five_prime = 1;
		}

		/* Finding the 3' cutoff */
		/* if the average quality in the window is less than the threshold */
		/* or if the window is the last window in the read */
		if ((window_avg < qual_threshold ||
			(size_t)(window_start+window_size) > fqrec.qual.length()) && (found_five_prime == 1 || no_fiveprime)) {

			/* at what point in the window does the quality dip below the threshold? */
			for (j=window_start; j<window_start+window_size; j++) {
				if (get_quality_num (fqrec.qual.at(j), fqrec, j) < qual_threshold) {
					three_prime_cut = j;
					break;
				}
			}

			break;
		}

		/* instead of sliding the window, subtract the first qual and add the next qual */
		window_total -= get_quality_num (fqrec.qual.at(window_start), fqrec, window_start);
		if ((size_t)(window_start+window_size) < fqrec.qual.length()) {
			window_total += get_quality_num (fqrec.qual.at(window_start+window_size), fqrec, window_start+window_size);
		}
		window_start++;
	}


    /* If truncate N option is selected, and sequence has Ns, then */
    /* change 3' cut site to be the base before the first N */
	size_t nIndex = fqrec.seq.find("n");
	size_t NIndex = fqrec.seq.find("N");
	bool hasN = false;
	if(nIndex != std::string::npos){
		npos = nIndex;
		hasN = true;
	}else if(NIndex != std::string::npos){
		npos = nIndex;
		hasN = true;
	}
    if (trunc_n && hasN) {
        three_prime_cut = npos - 1;
    }

    /* if cutting length is less than threshold then return -1 for both */
    /* to indicate that the read should be discarded */
    /* Also, if you never find a five prime cut site, then discard whole read */
    if ((found_five_prime == 0 && !no_fiveprime) || (three_prime_cut - five_prime_cut < length_threshold)) {
        three_prime_cut = -1;
        five_prime_cut = -1;

        if (debug) printf("%s\n", string(fqrec.name).c_str());
    }

    if (debug) printf ("\n\n");

	retvals = (cutsites*) malloc (sizeof(cutsites));
	retvals->three_prime_cut = three_prime_cut;
	retvals->five_prime_cut = five_prime_cut;
	return (retvals);
}

int Abstract_Trimmer::get_quality_num(char qualchar, FQEntry &fqrec, int pos){
  /*
     Return the adjusted quality, depending on quality type.

     Note that this uses the array in sickle.h, which *approximates*
     the SOLEXA (pre-1.3 pipeline) qualities as linear. This is
     inaccurate with low-quality bases.
  */

  int qual_value = (int) qualchar;

  if (qual_value < quality_constants[qualtype][Q_MIN] || qual_value > quality_constants[qualtype][Q_MAX]) {
	fprintf (stderr, "ERROR: Quality value (%d) does not fall within correct range for %s encoding.\n", qual_value, typenames[qualtype]);
	fprintf (stderr, "Range for %s encoding: %d-%d\n", typenames[qualtype], quality_constants[qualtype][Q_MIN], quality_constants[qualtype][Q_MAX]);
	fprintf (stderr, "FastQ record: %s\n", string(fqrec.name).c_str());
	fprintf (stderr, "Quality string: %s\n", string(fqrec.qual).c_str());
	fprintf (stderr, "Quality char: '%c'\n", qualchar);
	fprintf (stderr, "Quality position: %d\n", pos+1);
	exit(1);
  }

  return (qual_value - quality_constants[qualtype][Q_OFFSET]);
}