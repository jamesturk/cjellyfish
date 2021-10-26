#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "jellyfish.h"

/* borrowed heavily from strcmp95.c
 *    http://www.census.gov/geo/msb/stand/strcmp.c
 */
double _jaro_winkler(const JFISH_UNICODE *ying, int ying_length,
                     const JFISH_UNICODE *yang, int yang_length,
                     int long_tolerance, int winklerize)
{
    /* Arguments:

       ying
       yang
         pointers to the 2 strings to be compared.

       long_tolerance
         Increase the probability of a match when the number of matched
         characters is large.  This option allows for a little more
         tolerance when the strings are large.  It is not an appropriate
         test when comparing fixed length fields such as phone and
         social security numbers.
    */
    JFISH_UNICODE *ying_flag=0, *yang_flag=0;

    double weight;

    long min_len;
    long search_range;
    long lowlim, hilim;
    long trans_count, common_chars;

    int i, j, k;

    // ensure that neither string is blank
    if (!ying_length || !yang_length) return 0;

    if (ying_length > yang_length) {
        search_range = ying_length;
        min_len = yang_length;
    } else {
        search_range = yang_length;
        min_len = ying_length;
    }
  
    // Blank out the flags
    ying_flag = calloc((ying_length + 1), sizeof(JFISH_UNICODE));
    if (!ying_flag) {
        return -100;
    }

    yang_flag = calloc((yang_length + 1), sizeof(JFISH_UNICODE));
    if (!yang_flag) {
        free(ying_flag);
        return -100;
    }

    search_range = (search_range/2) - 1;
    if (search_range < 0) search_range = 0;


    // Looking only within the search range, count and flag the matched pairs.
    common_chars = 0;
    for (i = 0; i < ying_length; i++) {
        lowlim = (i >= search_range) ? i - search_range : 0;
        hilim = (i + search_range <= yang_length-1) ? (i + search_range) : yang_length-1;
        for (j = lowlim; j <= hilim; j++)  {
            if (!yang_flag[j] && yang[j] == ying[i]) {
                yang_flag[j] = 1;
                ying_flag[i] = 1;
                common_chars++;
                break;
            }
        }
    }

    // If no characters in common - return
    if (!common_chars) {
        free(ying_flag);
        free(yang_flag);
        return 0;
    }

    // Count the number of transpositions
    k = trans_count = 0;
    for (i = 0; i < ying_length; i++) {
        if (ying_flag[i]) {
            for (j = k; j < yang_length; j++) {
                if (yang_flag[j]) {
                    k = j + 1;
                    break;
                }
            }
            if (ying[i] != yang[j]) {
                trans_count++;
            }
        }
    }
    trans_count /= 2;

    // adjust for similarities in nonmatched characters

    // Main weight computation.
    weight= common_chars / ((double) ying_length) + common_chars / ((double) yang_length)
        + ((double) (common_chars - trans_count)) / ((double) common_chars);
    weight /=  3.0;

    // Continue to boost the weight if the strings are similar
    if (winklerize && weight > 0.7) {

        // Adjust for having up to the first 4 characters in common
        j = (min_len >= 4) ? 4 : min_len;
        for (i=0; ((i<j) && (ying[i] == yang[i])); i++);
        if (i) {
            weight += i * 0.1 * (1.0 - weight);
        }

        /* Optionally adjust for long strings. */
        /* After agreeing beginning chars, at least two more must agree and
           the agreeing characters must be > .5 of remaining characters.
        */
        if ((long_tolerance) && (min_len>4) && (common_chars>i+1) && (2*common_chars>=min_len+i)) {
            weight += (double) (1.0-weight) *
                ((double) (common_chars-i-1) / ((double) (ying_length+yang_length-i*2+2)));
        }
    }

    free(ying_flag);
    free(yang_flag);
    return weight;
}


double jaro_winkler_similarity(const JFISH_UNICODE *ying, int ying_len,
        const JFISH_UNICODE *yang, int yang_len,
        int long_tolerance)
{
    return _jaro_winkler(ying, ying_len, yang, yang_len, long_tolerance, 1);
}

double jaro_similarity(const JFISH_UNICODE *ying, int ying_len, const JFISH_UNICODE *yang, int yang_len)
{
    return _jaro_winkler(ying, ying_len, yang, yang_len, 0, 0);
}
