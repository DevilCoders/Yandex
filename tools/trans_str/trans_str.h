#pragma once

#include <contrib/libs/pcre/pcreposix.h>

#include <util/system/defaults.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

/********************************************************
 *                   (C) Sergey Trifonov, 2005
 *
 * This class is used to perform regex transformations
 * over strings.
 *
 * Usage example see in tools/trans_str utility
 *
 * The configuration file should have syntax:
 * Comment lines marked by '#' in the fist position, or empty line
 *
 * The whole content is split onto blocks of one match instruction and
 * list of replace subinstructions.
 *
 * Each instruction fits a line in the file. Fields in instructions
 * are separated by symbol '@', to use this symbol inside a field, type '\@'
 *
 * Instruction m@<pattern>@<options> matches the string, and applies
 *  to it the following replacement instructions:
 *    s@<pattern>@<replacement>@<options>
 * options:
 *   ! - final action (for both match and replace patterns)
 *   r - whole recoursion (for replacement only)
 *   s - semi-recoursion  (for replacement only,
 *                         each match repaced only once from left to right)
 *   i - ignorecase
 *   x - REG_EXTENDED
 *   f - REG_NOSUB (for match patters automatically is ON!)
 *   n - REG_NEWLINE
 *   ! - if matches, it is the last
 *              processing instruction (for replacement),
 *              or processing block (for match)
 *
 * If the option "debug_mode" on initialization is on, the programm
 * prints informative log to stdout. Using this log, one can investigate
 * all the transformations over the strings being processed.
 *
 * One can use his own logger: the method Log is overridable.
 * By default it reports errors to stderr.
 *
 *******************************************************/
/********************************************************/

class tsSelectPattern;
/********************************************************/

class tsStringTransformer
{
  protected:
    TVector<tsSelectPattern*> mPatterns;
    bool                     mOK;

    void badLine(const char* fName,
                 int nLine,
                 const char* rep);

    void addSelect(const char* opt,
                   int         cflags,
                   bool        final,
                   const char* fName,
                   int         nLine,
                   bool        debug);

    void addReplace(const char* opt,
                    const char* replacement,
                    int         recoursion,
                    int         cflags,
                    bool        final,
                    const char* fName,
                    int         nLine,
                    bool        debug);

    void finishUp  (const char* fName,
                    int         nLine);

    bool split(const char* orig,
               char*       buf,
               char**      sections,
               int         min_Sections,
               int         max_sections);

    void mkopts(char*      buf,
                int&       recoursion,
                int&       cflags,
                bool&      final);

  public:
    tsStringTransformer();
    virtual ~tsStringTransformer();

    bool prepare(const char* fName, bool debug);
    bool process(TString& str);

    bool isOK()
    {
        return mOK;
    }

    virtual void Log(const char* msg);
};

/********************************************************/
