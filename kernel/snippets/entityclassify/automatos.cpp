#include "automatos.h"

#include <util/string/split.h>
#include <util/charset/wide.h>
#include <util/stream/output.h>

static const wchar16 wideCyrillicAlphabet[] = {
        0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438,
0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
        0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448,
0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F, 0x00 };



static bool PerfectiveMatch(TUtf16String& word) {

    enum ESTATES {
        START,
        FIRSTI,
        FIRSTV,
        FIRST1,
        SECONDS,
        SECONDSH,
        END
    };

    ESTATES state = START;
    size_t delit = word.size();

    for (int it = word.size() - 1; it >= 0; --it) {
        if (state == START) {
             if ( word[it] ==  wideCyrillicAlphabet[8])
                   state = FIRSTI;
             else if (word[it] == wideCyrillicAlphabet[2])
                   state = FIRSTV;
             else if (word[it] == wideCyrillicAlphabet[28])
                   state = FIRST1;
             else
                   return false;
            continue;
        }

        if (state == FIRSTI) {
            if (word[it] == wideCyrillicAlphabet[24])
                 state = SECONDSH;
            else
                 return false;
            continue;
        }

        if (state == FIRST1) {
            if (word[it] == wideCyrillicAlphabet[17])
                 state = SECONDS;
            else
                 return false;
            continue;
        }

        if (state == SECONDSH ) {
            if (word[it] == wideCyrillicAlphabet[2])
                 state = FIRSTV;
            else
                 return false;
            continue;
        }

        if (state == SECONDS) {
            if (word[it] == wideCyrillicAlphabet[8])
                 state = FIRSTI;
            else
                 return false;
            continue;
        }

       if (state == FIRSTV) {
           if ((word[it] == wideCyrillicAlphabet[0]) || (word[it] == wideCyrillicAlphabet[31])) {
               state = END;
               delit = it+1;
               break;
           } else if ((word[it] == wideCyrillicAlphabet[8]) || (word[it] == wideCyrillicAlphabet[27])) {
               state = END;
               delit = it;
               break;
           } else
               return false;
       }
    }

    if (state == END) {
        word =  word.substr(0, delit);
        return true;
    }

    return false;
}

static bool ReflexiveMatch(TUtf16String& word) {

    size_t it = word.size();
    if (it < 2)
        return false;

    if ((word[it-2] != wideCyrillicAlphabet[17]) || (word[it-1] != wideCyrillicAlphabet[31]) && (word[it-1] != wideCyrillicAlphabet[28]))
        return false;

    word = word.substr(0, it-2);
    return true;

}

static bool AdjectiveMatch(TUtf16String& word) {

    enum ESTATES {
        START,
        FIRSTEIM,
        FIRSTO,
        FIRSTU,
        FIRSTYU,
        FIRSTYA,
        FIRSTI,
        FIRSTH,
        SECM1,
        SECM2,
        SECG,
        END
    };

    ESTATES state = START;
    size_t delit = word.size();

    for (int it = word.size() - 1; it >= 0; --it) {
        if (state == START) {
             if ((word[it] ==  wideCyrillicAlphabet[5]) || (word[it] ==  wideCyrillicAlphabet[9]) || (word[it] ==  wideCyrillicAlphabet[12]))
                   state = FIRSTEIM;
             else if (word[it] == wideCyrillicAlphabet[14])
                   state = FIRSTO;
             else if (word[it] == wideCyrillicAlphabet[19])
                   state = FIRSTU;
             else if (word[it] == wideCyrillicAlphabet[30])
                   state = FIRSTYU;
             else if (word[it] == wideCyrillicAlphabet[31])
                   state = FIRSTYA;
             else if (word[it] == wideCyrillicAlphabet[8])
                   state = FIRSTI;
             else if (word[it] == wideCyrillicAlphabet[21])
                   state = FIRSTH;
             else
                   return false;
            continue;
        }

        if (state == FIRSTO) {
            if (word[it] == wideCyrillicAlphabet[3])
                 state = SECG;
            else
                 return false;
            continue;
        }

        if (state == FIRSTU) {
            if (word[it] == wideCyrillicAlphabet[12])
                 state = SECM1;
            else
                 return false;
            continue;
        }

        if (state == FIRSTI) {
            if (word[it] == wideCyrillicAlphabet[12])
                 state = SECM2;
            else
                 return false;
            continue;
        }

       if ((state == SECG) || (state == SECM1) || (state == FIRSTEIM) || (state == FIRSTYU)) {
           if ((word[it] == wideCyrillicAlphabet[14]) || (word[it] == wideCyrillicAlphabet[5])) {
               state = END;
               delit = it;
               break;
           } else if ((state == SECG) || (state == SECM1))
               return false;
       }

       if ((state == SECM2) || (state == FIRSTH) || (state == FIRSTEIM)) {
           if ((word[it] == wideCyrillicAlphabet[8]) || (word[it] == wideCyrillicAlphabet[27])) {
               state = END;
               delit = it;
               break;
           } else
               return false;
       }

       if (state == FIRSTYU) {
          if ((word[it] == wideCyrillicAlphabet[19]) || (word[it] == wideCyrillicAlphabet[30])) {
               state = END;
               delit = it;
               break;
          } else
              return false;
       }

       if (state == FIRSTYA) {
          if ((word[it] == wideCyrillicAlphabet[0]) || (word[it] == wideCyrillicAlphabet[31])) {
               state = END;
               delit = it;
               break;
          } else
              return false;
       }

    }

    if (state == END) {
        word =  word.substr(0, delit);
        return true;
    }

    return false;

}


static bool ParticleMatch(TUtf16String& word) {

    size_t tt = word.size();
    if (tt < 2)
        return false;
    wchar16 last = word[tt-1];
    wchar16 prlast = word[tt-2];

    if ( ((prlast == wideCyrillicAlphabet[0]) || (prlast == wideCyrillicAlphabet[31])) && (last == wideCyrillicAlphabet[25])) {
        word = word.substr(0,tt-1);
        return true;
    }

    if (tt < 3)
       return false;

    wchar16 twolast = word[tt-3];

    if ((twolast == wideCyrillicAlphabet[0]) || (twolast == wideCyrillicAlphabet[31]))
        if ( (last == wideCyrillicAlphabet[12]) && (prlast == wideCyrillicAlphabet[5]) ||
             (last == wideCyrillicAlphabet[25]) && (prlast == wideCyrillicAlphabet[30]) ||
             (last == wideCyrillicAlphabet[13]) && (prlast == wideCyrillicAlphabet[13]) ) {
             word = word.substr(0,tt-2);
             return true;
        }


    if ((twolast == wideCyrillicAlphabet[8]) || (twolast == wideCyrillicAlphabet[27]))
        if ( (last == wideCyrillicAlphabet[24]) && (prlast == wideCyrillicAlphabet[2])) {
            word = word.substr(0, tt-3);
            return true;
        }

    if ((twolast == wideCyrillicAlphabet[19]) && (prlast == wideCyrillicAlphabet[30]) && (last == wideCyrillicAlphabet[25])) {
            word = word.substr(0, tt-3);
            return true;
    }

    return false;
}


static bool VerbMatch(TUtf16String& word) {

    enum ESTATES {
        START,
        FIRSTI,
        FIRSTAO,
        FIRSTII,
        FIRSTE,
        FIRSTL,
        FIRSTN,
        FIRSTI1,
        FIRSTT,
        FIRSTYU,
        FIRST1,
        FIRSTM,
        SECE,
        SECSH,
        SECEYU,
        SECT1,
        SECT2,
        END
    };

    ESTATES state = START;
    size_t delit = word.size();

    for (int it = word.size() - 1; it >= 0; --it) {
       if (state == START) {
             if ((word[it] ==  wideCyrillicAlphabet[0]) || (word[it] ==  wideCyrillicAlphabet[14]))
                   state = FIRSTAO;
             else if (word[it] == wideCyrillicAlphabet[8])
                   state = FIRSTI;
             else if (word[it] == wideCyrillicAlphabet[27])
                   state = FIRSTII;
             else if (word[it] == wideCyrillicAlphabet[5])
                   state = FIRSTE;
             else if (word[it] == wideCyrillicAlphabet[11])
                   state = FIRSTL;
             else if (word[it] == wideCyrillicAlphabet[13])
                   state = FIRSTN;
             else if (word[it] == wideCyrillicAlphabet[9])
                   state = FIRSTI1;
             else if (word[it] == wideCyrillicAlphabet[18])
                   state = FIRSTT;
             else if (word[it] == wideCyrillicAlphabet[30])
                   state = FIRSTYU;
             else if (word[it] == wideCyrillicAlphabet[28])
                   state = FIRST1;
             else if (word[it] == wideCyrillicAlphabet[12])
                   state = FIRSTM;
             else
                   return false;
            continue;
        }

        if (state == FIRSTAO) {
             if (word[it] == wideCyrillicAlphabet[11])
                   state = FIRSTL;
             else if (word[it] == wideCyrillicAlphabet[13])
                   state = FIRSTN;
             else
                   return false;
             continue;
        }

        if (state == FIRSTI) {
             if (word[it] == wideCyrillicAlphabet[11])
                  state = FIRSTL;
             else
                  return false;
             continue;
        }

        if (state == FIRSTII) {
            if (word[it] == wideCyrillicAlphabet[13])
                  state = FIRSTN;
            else
                  return false;
            continue;
        }

        if (state == FIRSTL) {
            if ((word[it] == wideCyrillicAlphabet[8]) || (word[it] == wideCyrillicAlphabet[27]))
                delit = it;
            else if ((word[it] == wideCyrillicAlphabet[0]) || (word[it] == wideCyrillicAlphabet[31]))
                delit = it + 1;
            else
                return false;
            state = END;
            break;
        }

        if (state == FIRSTN) {
            if (word[it] == wideCyrillicAlphabet[5])
                delit = it;
            else if ((word[it] == wideCyrillicAlphabet[0]) || (word[it] == wideCyrillicAlphabet[31]))
                delit = it + 1;
            else
                return false;
            state = END;
            break;
        }

        if (state == FIRSTI1) {
            if ((word[it] == wideCyrillicAlphabet[5]) || (word[it] == wideCyrillicAlphabet[19]))
                delit = it;
            else if ((word[it] == wideCyrillicAlphabet[0]) || (word[it] == wideCyrillicAlphabet[31]))
                delit = it + 1;
            else
                return false;
            state = END;
            break;
        }

        if (state == FIRSTYU) {
           if (word[it] == wideCyrillicAlphabet[19])
               delit = it;
           else
               delit = it + 1;
           state = END;
           break;
        }

       if (state == FIRSTM) {
           if ((word[it] == wideCyrillicAlphabet[8]) || (word[it] == wideCyrillicAlphabet[27]))
                delit = it;
           else if (word[it] == wideCyrillicAlphabet[5]) {
                state = SECE;
                continue;
           } else
                return false;
           state = END;
           break;
       }

       if (state == FIRST1) {
          if (word[it] == wideCyrillicAlphabet[24])
               state = SECSH;
          else if (word[it] == wideCyrillicAlphabet[18])
               state = SECT1;
          else
               return false;
          continue;
       }

       if (state == FIRSTT) {
          if ((word[it] == wideCyrillicAlphabet[5]) || (word[it] == wideCyrillicAlphabet[30]))
              state = SECEYU;
          else if ((word[it] == wideCyrillicAlphabet[8]) || (word[it] == wideCyrillicAlphabet[27]) ||  (word[it] == wideCyrillicAlphabet[31])) {
              delit = it;
              state = END;
              break;
          } else
              return false;
          continue;
       }

       if (state == FIRSTE) {
           if (word[it] == wideCyrillicAlphabet[18])
               state = SECT2;
           else
               return false;
           continue;
       }

       if (state == SECE) {
            if ((word[it] == wideCyrillicAlphabet[0]) || (word[it] == wideCyrillicAlphabet[31]))
                delit = it +  1;
            else
                return false;
            state = END;
            break;
       }

       if (state == SECSH) {
           if (word[it] == wideCyrillicAlphabet[5])
               state = SECE;
           else if (word[it] == wideCyrillicAlphabet[8]) {
               delit = it;
               state = END;
               break;
           } else
               return false;
          continue;
       }

      if (state == SECT1) {
           if ((word[it] == wideCyrillicAlphabet[8]) || (word[it] == wideCyrillicAlphabet[27]))
              delit = it;
           else if ((word[it] == wideCyrillicAlphabet[0]) || (word[it] == wideCyrillicAlphabet[31]))
              delit = it + 1;
           else
              return  false;

           state = END;
           break;
      }

     if (state == SECEYU) {
            if (word[it] == wideCyrillicAlphabet[19])
                delit = it;
            else if ((word[it] == wideCyrillicAlphabet[0]) || (word[it] == wideCyrillicAlphabet[31]))
                delit = it + 1;
            else
                return false;
            state = END;
            break;
     }

     if (state == SECT2) {
           if (word[it] == wideCyrillicAlphabet[8]) {
                state = END;
                delit = it;
                break;
           } else if (word[it] == wideCyrillicAlphabet[5])
                state = SECE;
           else if (word[it] == wideCyrillicAlphabet[9])
                state = FIRSTI1;
           else
                return false;
           continue;
     }


    }

    if (state == FIRSTYU) {
        state = END;
        delit = 0;
    }

    if (state == END) {
        word =  word.substr(0, delit);
        return true;
    }

    return false;

}

static bool NounMatch(TUtf16String& word) {

    enum ESTATES {
        START,
        FIRSTEYUYA,
        FIRSTI,
        FIRSTI1,
        FIRSTV,
        FIRSTH,
        FIRSTM,
        WAITI,
        END
    };

    ESTATES state = START;
    size_t delit = word.size();

    for (int it = word.size() - 1; it >= 0; --it) {
       if (state == START) {
             if ( (word[it] ==  wideCyrillicAlphabet[27]) || (word[it] ==  wideCyrillicAlphabet[28])  || (word[it] ==  wideCyrillicAlphabet[0]) || (word[it] ==  wideCyrillicAlphabet[14]) || (word[it] ==  wideCyrillicAlphabet[19])) {
                 delit = it;
                 state = END;
                 break;
             } else if ((word[it] ==  wideCyrillicAlphabet[5]) || (word[it] ==  wideCyrillicAlphabet[30]) || (word[it] ==  wideCyrillicAlphabet[31]))
                 state = FIRSTEYUYA;
             else if (word[it] ==  wideCyrillicAlphabet[8])
                 state = FIRSTI;
             else if (word[it] == wideCyrillicAlphabet[9])
                 state = FIRSTI1;
             else if (word[it] == wideCyrillicAlphabet[2])
                 state = FIRSTV;
             else if (word[it] == wideCyrillicAlphabet[12])
                 state = FIRSTM;
             else if (word[it] == wideCyrillicAlphabet[21])
                 state = FIRSTH;
             else
                 return false;
             continue;
        }

       if (state == FIRSTEYUYA) {
           if ((word[it] ==  wideCyrillicAlphabet[8]) || (word[it] ==  wideCyrillicAlphabet[28]))
               delit = it;
           else
               delit = it + 1;
           state = END;
           break;
       }

       if (state == FIRSTI) {
           if ((word[it] == wideCyrillicAlphabet[8]) || (word[it] == wideCyrillicAlphabet[5]))
               delit = it;
           else if (word[it] == wideCyrillicAlphabet[12]) {
                  state = FIRSTH;
               continue;
           } else
               delit = it + 1;

           state = END;
           break;
       }

       if (state == FIRSTI1) {
           if ((word[it] == wideCyrillicAlphabet[14]) || (word[it] == wideCyrillicAlphabet[8]))
               delit = it;
           else if (word[it] == wideCyrillicAlphabet[5]) {
               state = WAITI;
               continue;
           } else
               delit = it + 1;
           state = END;
           break;
       }

       if (state == FIRSTM) {
           if ((word[it] == wideCyrillicAlphabet[14]) || (word[it] == wideCyrillicAlphabet[0]))
               delit = it;
           else if ((word[it] == wideCyrillicAlphabet[31]) || (word[it] == wideCyrillicAlphabet[5])) {
               state = WAITI;
               continue;
           } else
               return false;
           state = END;
           break;
       }

      if (state == FIRSTH) {
           if (word[it] == wideCyrillicAlphabet[0])
               delit = it;
           else if (word[it] == wideCyrillicAlphabet[31]) {
               state = WAITI;
               continue;
           } else
               return false;
           state = END;
           break;
      }

      if (state == FIRSTV) {
          if ((word[it] == wideCyrillicAlphabet[14]) || (word[it] == wideCyrillicAlphabet[5]))
              delit = it;
          else
              return false;
          state = END;
          break;
      }

      if (state == WAITI) {
          if (word[it] == wideCyrillicAlphabet[8])
              delit = it;
          else
              delit = it + 1;
          state = END;
          break;
      }
    }

    if ((state == WAITI) || (state == FIRSTEYUYA) || (state == FIRSTI1) || (state == FIRSTI)) {
        state = END;
        delit = 0;
    }

    if (state == END) {
        word =  word.substr(0, delit);
        return true;
    }

    return false;

}

static bool IsGlasnaya(wchar16 letter) {

    ui32 mask = 1u << (letter - wideCyrillicAlphabet[0]);

    const ui32 glassMask = 1u | (1u << 5) | (1u << 8) | (1u << 14) | (1u << 19) | (1u << 27) | (1u << 29) | (1u << 30) | (1u << 31);

    if (mask & glassMask)
        return true;

    return false;
}

static bool DerviationalMatch(TUtf16String& word) {

     size_t tt = word.size();
     if (tt < 6)
         return false;

     if (!( (word[tt-1] == wideCyrillicAlphabet[28]) && (word[tt-2] == wideCyrillicAlphabet[18]) && (word[tt-3] == wideCyrillicAlphabet[17]) && (word[tt-4] == wideCyrillicAlphabet[14])))
         return false;

     for (size_t it = 0; it < word.size() - 5; ++it)
         if (!IsGlasnaya(word[it]) && IsGlasnaya(word[it + 1]) ) {
             word = word.substr(0, tt - 4);
             return true;
         }

     return false;
}

TUtf16String PorterStemm(TUtf16String word) {

   size_t pos = word.size();

   for (size_t i = 0; i < word.size(); ++i)
       if (IsGlasnaya(word[i])) {
           pos = i+1;
           break;
       }

   if (pos >=  word.size())
       return word;


   TUtf16String first = word.substr(0, pos);
   TUtf16String second = word.substr(pos, word.size());

   if (!PerfectiveMatch(second)) {
       ReflexiveMatch(second);

       if (AdjectiveMatch(second)) {
           ParticleMatch(second);
       } else {
           if (!VerbMatch(second)) {
               NounMatch(second);
           }
       }
   }

   if ((second.size() > 0) && (second.back() == wideCyrillicAlphabet[8]))
       second = second.substr(0, second.size() - 1);

   DerviationalMatch(second);

   if (second.size() > 0) {
       if (second.back() == wideCyrillicAlphabet[28])
           second = second.substr(0, second.size() - 1);
       else {
           size_t tt = second.size();
           if ((tt > 3) && ( (second[tt-1] == wideCyrillicAlphabet[5]) && (second[tt-2] == wideCyrillicAlphabet[24]) && (second[tt-3] == wideCyrillicAlphabet[9]) && (second[tt-4] == wideCyrillicAlphabet[5
])))
               second = second.substr(0, second.size() - 4);
           else if ((tt > 2) && ((second[tt-1] == wideCyrillicAlphabet[24]) && (second[tt-2] == wideCyrillicAlphabet[9]) && (second[tt-3] == wideCyrillicAlphabet[5])))
               second = second.substr(0, second.size() - 3);

           tt = second.size();
           if ((tt > 1) && ((second[tt-1] == wideCyrillicAlphabet[13]) && (second[tt-2] == wideCyrillicAlphabet[13])))
               second = second.substr(0, second.size() - 1);
       }

   }

   return first + second;
}

