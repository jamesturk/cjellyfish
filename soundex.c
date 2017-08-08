#include "jellyfish.h"
#include <ctype.h>
#include <stdlib.h>

// This implementation is based on the soundex implemenation in the R
// stringdist package (https://github.com/markvanderloo/stringdist/blob/master
// /pkg/src/soundex.c). Stringdist is distributed with the GPL3 license. See
// https://github.com/markvanderloo/stringdist for the complete license. 


// Translate similar sounding consonants to numeric codes; vowels are all 
// translated to 'a' and voiceless characters (and other characters) are 
// translated to 'h'.
// Upper and lower case ASCII characters are treated as separate cases,
// avoiding the use of 'tolower' whose effect depends on locale.
static unsigned int translate_soundex(char c) {
  switch ( c ) {
    case 'b':
    case 'f':
    case 'p':
    case 'v':
    case 'B':
    case 'F':
    case 'P':
    case 'V':
      return '1';
    case 'c':
    case 'g':
    case 'j':
    case 'k':
    case 'q':
    case 's':
    case 'x':
    case 'z':
    case 'C':
    case 'G':
    case 'J':
    case 'K':
    case 'Q':
    case 'S':
    case 'X':
    case 'Z':
      return '2';
    case 'd':
    case 't':
    case 'D':
    case 'T':
      return '3';
    case 'l':
    case 'L':
      return '4';
    case 'm':
    case 'n':
    case 'M':
    case 'N':
      return '5';
    case 'r':
    case 'R':
      return '6';
    case 'h':
    case 'w':
    case 'H':
    case 'W':
      return 'h';
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
    case 'y':
    case 'A':
    case 'E':
    case 'I':
    case 'O':
    case 'U':
    case 'Y':
      return 'a'; // use 'a' to encode vowels
    case '!': // we will allow all printable ASCII characters.
    case '"':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case '-':
    case '.':
    case '/':
    case ':':
    case ';':
    case '<':
    case '=':
    case '>':
    case '?':
    case '@':
    case '[':
    case '\\':
    case ']':
    case '^':
    case '_':
    case '`':
    case '{':
    case '|':
    case '}':
    case '~':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case ' ':
      return 'h'; // ignored characters; voiceless symbols.
    default:
      return '?'; // other characters are ignored with a warning
  }
}


char* soundex(const char *str, size_t str_len)
{
    unsigned int i = 0, j = 0;
    char *result = calloc(5, sizeof(char));

    if (!result) {
        return NULL;
    }

    if (!*str) {
        return result;
    }

    unsigned int cj = translate_soundex(str[0]);
    // the first character is copied directly and not translated to a numerical
    // code
    if ( cj == '?' ){
        // the translated character is non-printable ASCII or non-ASCII.
        result[0] = str[0];
    } else {
        result[0] = toupper(str[0]);
    }

    for (i = 1; i < str_len && j < 3; ++i) {

        unsigned int ci = translate_soundex(str[i]);
        if (ci == 'a') {
            // vowels are not added to the result; but we do set the previous
            // character to the vower because two consonants with a vowel in between
            // are not merged
            cj = ci;
        } else if (ci != 'h') {
            // a consonant that is not equal to the previous consonant is added to 
            // the result
            if (ci != cj) {
                result[++j] = ci;
                cj = ci;
            }
        }
    }

    for (++j ; j < 4; ++j) result[j] = '0';

    return result;
}
