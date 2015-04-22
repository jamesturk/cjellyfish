/* Copyright Martin Porter, released under BSD compatible licensing terms.
   See http://tartarus.org/~martin/PorterStemmer/ for details.
*/

/* This is the Porter stemming algorithm, coded up as thread-safe ANSI C
   by the author.

   It may be be regarded as cononical, in that it follows the algorithm
   presented in

   Porter, 1980, An algorithm for suffix stripping, Program, Vol. 14,
   no. 3, pp 130-137,

   only differing from it at the points maked --DEPARTURE-- below.

   See also http://www.tartarus.org/~martin/PorterStemmer

   The algorithm as described in the paper could be exactly replicated
   by adjusting the points of DEPARTURE, but this is barely necessary,
   because (a) the points of DEPARTURE are definitely improvements, and
   (b) no encoding of the Porter stemmer I have seen is anything like
   as exact as this version, even with the points of DEPARTURE!

   You can compile it on Unix with 'gcc -O3 -o stem stem.c' after which
   'stem' takes a list of inputs and sends the stemmed equivalent to
   stdout.

   The algorithm as encoded here is particularly fast.

   Release 2 (the more old-fashioned, non-thread-safe version may be
   regarded as release 1.)
*/

#include <stdlib.h>  /* for malloc, free */
#include "jellyfish.h"

/* You will probably want to move the following declarations to a central
   header file.
*/

struct stemmer;

extern struct stemmer * create_stemmer(void);
extern void free_stemmer(struct stemmer * z);

extern int stem(struct stemmer * z, JFISH_UNICODE * b, int k);



/* The main part of the stemming algorithm starts here.
*/

#define TRUE 1
#define FALSE 0

/* stemmer is a structure for a few local bits of data,
*/

struct stemmer {
   JFISH_UNICODE * b;       /* buffer for word to be stemmed */
   int k;          /* offset to the end of the string */
   int j;          /* a general offset into the string */
};


/* Member b is a buffer holding a word to be stemmed. The letters are in
   b[0], b[1] ... ending at b[z->k]. Member k is readjusted downwards as
   the stemming progresses. Zero termination is not in fact used in the
   algorithm.

   Note that only lower case sequences are stemmed. Forcing to lower case
   should be done before stem(...) is called.


   Typical usage is:

       struct stemmer * z = create_stemmer();
       char b[] = "pencils";
       int res = stem(z, b, 6);
           /- stem the 7 characters of b[0] to b[6]. The result, res,
              will be 5 (the 's' is removed). -/
       free_stemmer(z);
*/


extern struct stemmer * create_stemmer(void)
{
    return (struct stemmer *) malloc(sizeof(struct stemmer));
    /* assume malloc succeeds */
}

extern void free_stemmer(struct stemmer * z)
{
    free(z);
}


/* cons(z, i) is TRUE <=> b[i] is a consonant. ('b' means 'z->b', but here
   and below we drop 'z->' in comments.
*/

static int cons(struct stemmer * z, int i)
{  switch (z->b[i])
   {  case 'a': case 'e': case 'i': case 'o': case 'u': return FALSE;
      case 'y': return (i == 0) ? TRUE : !cons(z, i - 1);
      default: return TRUE;
   }
}

/* m(z) measures the number of consonant sequences between 0 and j. if c is
   a consonant sequence and v a vowel sequence, and <..> indicates arbitrary
   presence,

      <c><v>       gives 0
      <c>vc<v>     gives 1
      <c>vcvc<v>   gives 2
      <c>vcvcvc<v> gives 3
      ....
*/

static int m(struct stemmer * z)
{  int n = 0;
   int i = 0;
   int j = z->j;
   while(TRUE)
   {  if (i > j) return n;
      if (! cons(z, i)) break; i++;
   }
   i++;
   while(TRUE)
   {  while(TRUE)
      {  if (i > j) return n;
            if (cons(z, i)) break;
            i++;
      }
      i++;
      n++;
      while(TRUE)
      {  if (i > j) return n;
         if (! cons(z, i)) break;
         i++;
      }
      i++;
   }
}

/* vowelinstem(z) is TRUE <=> 0,...j contains a vowel */

static int vowelinstem(struct stemmer * z)
{
   int j = z->j;
   int i; for (i = 0; i <= j; i++) if (! cons(z, i)) return TRUE;
   return FALSE;
}

/* doublec(z, j) is TRUE <=> j,(j-1) contain a double consonant. */

static int doublec(struct stemmer * z, int j)
{
   JFISH_UNICODE * b = z->b;
   if (j < 1) return FALSE;
   if (b[j] != b[j - 1]) return FALSE;
   return cons(z, j);
}

/* cvc(z, i) is TRUE <=> i-2,i-1,i has the form consonant - vowel - consonant
   and also if the second c is not w,x or y. this is used when trying to
   restore an e at the end of a short word. e.g.

      cav(e), lov(e), hop(e), crim(e), but
      snow, box, tray.

*/

static int cvc(struct stemmer * z, int i)
{  if (i < 2 || !cons(z, i) || cons(z, i - 1) || !cons(z, i - 2)) return FALSE;
   {  int ch = z->b[i];
      if (ch  == 'w' || ch == 'x' || ch == 'y') return FALSE;
   }
   return TRUE;
}

/* ends(z, s) is TRUE <=> 0,...k ends with the string s. */

static int ends(struct stemmer * z, int length, char * s)
{
   JFISH_UNICODE * b = z->b;
   int k = z->k;
   int i;
   if (s[length-1] != b[k]) return FALSE; /* tiny speed-up */
   if (length > k + 1) return FALSE;

   for(i=0; i < length; ++i) {
       if( *(b + k - length + 1 + i) != *(s + i) ) {
           return FALSE;
       }
   }
   z->j = k-length;
   return TRUE;
}

/* setto(z, s) sets (j+1),...k to the characters in the string s, readjusting
   k. */

static void setto(struct stemmer * z, int length, char * s)
{
   int j = z->j;
   int i;
   for(i=0; i < length; ++i) {
       *(z->b + j + 1 + i) = *(s + i);
   }
   z->k = j+length;
}

/* r(z, s) is used further down. */

static void r(struct stemmer * z, int len, char * s) { if (m(z) > 0) setto(z, len, s); }

/* step1ab(z) gets rid of plurals and -ed or -ing. e.g.

       caresses  ->  caress
       ponies    ->  poni
       ties      ->  ti
       caress    ->  caress
       cats      ->  cat

       feed      ->  feed
       agreed    ->  agree
       disabled  ->  disable

       matting   ->  mat
       mating    ->  mate
       meeting   ->  meet
       milling   ->  mill
       messing   ->  mess

       meetings  ->  meet

*/

static void step1ab(struct stemmer * z)
{
   JFISH_UNICODE * b = z->b;
   if (b[z->k] == 's')
   {  if (ends(z, 4, "sses")) z->k -= 2; else
      if (ends(z, 3, "ies")) setto(z, 1, "i"); else
      if (b[z->k - 1] != 's') z->k--;
   }
   if (ends(z, 3, "eed")) { if (m(z) > 0) z->k--; } else
   if ((ends(z, 2, "ed") || ends(z, 3, "ing")) && vowelinstem(z))
   {  z->k = z->j;
      if (ends(z, 2, "at")) setto(z, 3, "ate"); else
      if (ends(z, 2, "bl")) setto(z, 3, "ble"); else
      if (ends(z, 2, "iz")) setto(z, 3, "ize"); else
      if (doublec(z, z->k))
      {  z->k--;
         {  int ch = b[z->k];
            if (ch == 'l' || ch == 's' || ch == 'z') z->k++;
         }
      }
      else if (m(z) == 1 && cvc(z, z->k)) setto(z, 1, "e");
   }
}

/* step1c(z) turns terminal y to i when there is another vowel in the stem. */

static void step1c(struct stemmer * z)
{
   if (ends(z, 1, "y") && vowelinstem(z)) z->b[z->k] = 'i';
}


/* step2(z) maps double suffices to single ones. so -ization ( = -ize plus
   -ation) maps to -ize etc. note that the string before the suffix must give
   m(z) > 0. */

static void step2(struct stemmer * z) { switch (z->b[z->k-1])
{
   case 'a': if (ends(z, 7, "ational")) { r(z, 3, "ate"); break; }
             if (ends(z, 6, "tional")) { r(z, 4, "tion"); break; }
             break;
   case 'c': if (ends(z, 4, "enci")) { r(z, 4, "ence"); break; }
             if (ends(z, 4, "anci")) { r(z, 4, "ance"); break; }
             break;
   case 'e': if (ends(z, 4, "izer")) { r(z, 3, "ize"); break; }
             break;
   case 'l': if (ends(z, 3, "bli")) { r(z, 3, "ble"); break; } /*-DEPARTURE-*/

 /* To match the published algorithm, replace this line with
    case 'l': if (ends(z, 4, "abli")) { r(z, 4, "able"); break; } */

             if (ends(z, 4, "alli")) { r(z, 2, "al"); break; }
             if (ends(z, 5, "entli")) { r(z, 3, "ent"); break; }
             if (ends(z, 3, "eli")) { r(z, 1, "e"); break; }
             if (ends(z, 5, "ousli")) { r(z, 3, "ous"); break; }
             break;
   case 'o': if (ends(z, 7, "ization")) { r(z, 3, "ize"); break; }
             if (ends(z, 5, "ation")) { r(z, 3, "ate"); break; }
             if (ends(z, 4, "ator")) { r(z, 3, "ate"); break; }
             break;
   case 's': if (ends(z, 5, "alism")) { r(z, 2, "al"); break; }
             if (ends(z, 7, "iveness")) { r(z, 3, "ive"); break; }
             if (ends(z, 7, "fulness")) { r(z, 3, "ful"); break; }
             if (ends(z, 7, "ousness")) { r(z, 3, "ous"); break; }
             break;
   case 't': if (ends(z, 5, "aliti")) { r(z, 2, "al"); break; }
             if (ends(z, 5, "iviti")) { r(z, 3, "ive"); break; }
             if (ends(z, 6, "biliti")) { r(z, 3, "ble"); break; }
             break;
   case 'g': if (ends(z, 4, "logi")) { r(z, 3, "log"); break; } /*-DEPARTURE-*/

 /* To match the published algorithm, delete this line */

} }

/* step3(z) deals with -ic-, -full, -ness etc. similar strategy to step2. */

static void step3(struct stemmer * z) { switch (z->b[z->k])
{
   case 'e': if (ends(z, 5, "icate")) { r(z, 2, "ic"); break; }
             if (ends(z, 5, "ative")) { r(z, 0, ""); break; }
             if (ends(z, 5, "alize")) { r(z, 2, "al"); break; }
             break;
   case 'i': if (ends(z, 5, "iciti")) { r(z, 2, "ic"); break; }
             break;
   case 'l': if (ends(z, 4, "ical")) { r(z, 2, "ic"); break; }
             if (ends(z, 3, "ful")) { r(z, 0, ""); break; }
             break;
   case 's': if (ends(z, 4, "ness")) { r(z, 0, ""); break; }
             break;
} }

/* step4(z) takes off -ant, -ence etc., in context <c>vcvc<v>. */

static void step4(struct stemmer * z)
{  switch (z->b[z->k-1])
   {  case 'a': if (ends(z, 2, "al")) break; return;
      case 'c': if (ends(z, 4, "ance")) break;
                if (ends(z, 4, "ence")) break; return;
      case 'e': if (ends(z, 2, "er")) break; return;
      case 'i': if (ends(z, 2, "ic")) break; return;
      case 'l': if (ends(z, 4, "able")) break;
                if (ends(z, 4, "ible")) break; return;
      case 'n': if (ends(z, 3, "ant")) break;
                if (ends(z, 5, "ement")) break;
                if (ends(z, 4, "ment")) break;
                if (ends(z, 3, "ent")) break; return;
      case 'o': if (ends(z, 3, "ion") && (z->b[z->j] == 's' || z->b[z->j] == 't')) break;
                if (ends(z, 2, "ou")) break; return;
                /* takes care of -ous */
      case 's': if (ends(z, 3, "ism")) break; return;
      case 't': if (ends(z, 3, "ate")) break;
                if (ends(z, 3, "iti")) break; return;
      case 'u': if (ends(z, 3, "ous")) break; return;
      case 'v': if (ends(z, 3, "ive")) break; return;
      case 'z': if (ends(z, 3, "ize")) break; return;
      default: return;
   }
   if (m(z) > 1) z->k = z->j;
}

/* step5(z) removes a final -e if m(z) > 1, and changes -ll to -l if
   m(z) > 1. */

static void step5(struct stemmer * z)
{
   JFISH_UNICODE * b = z->b;
   z->j = z->k;
   if (b[z->k] == 'e')
   {  int a = m(z);
      if (a > 1 || (a == 1 && !cvc(z, z->k - 1))) z->k--;
   }
   if (b[z->k] == 'l' && doublec(z, z->k) && m(z) > 1) z->k--;
}

/* In stem(z, b, k), b is a char pointer, and the string to be stemmed is
   from b[0] to b[k] inclusive.  Possibly b[k+1] == '\0', but it is not
   important. The stemmer adjusts the characters b[0] ... b[k] and returns
   the new end-point of the string, k'. Stemming never increases word
   length, so 0 <= k' <= k.
*/

extern int stem(struct stemmer * z, JFISH_UNICODE * b, int k)
{
   if (k <= 1) return k; /*-DEPARTURE-*/
   z->b = b; z->k = k; /* copy the parameters into z */

   /* With this line, strings of length 1 or 2 don't go through the
      stemming process, although no mention is made of this in the
      published algorithm. Remove the line to match the published
      algorithm. */

   step1ab(z); step1c(z); step2(z); step3(z); step4(z); step5(z);
   return z->k;
}
