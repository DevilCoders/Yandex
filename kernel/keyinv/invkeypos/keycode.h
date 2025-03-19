#pragma once

#include "keychars.h"

#include <kernel/search_types/search_types.h>  // MAXKEY_LEN, MAXKEY_BUF

#include <library/cpp/wordpos/wordpos.h>
#include <library/cpp/langs/langs.h>

#include <util/string/hex.h>
#include <util/system/yassert.h>
#include <util/generic/noncopyable.h>

#include <cstring>


//! @note parts of key are stored into this structure in the following order:
//!             |       no prefix      |             with prefix
//!             | szPrefix    szLemma  | szPrefix                    szLemma
//! ------------+----------------------+--------------------------------------
//!   lemma     | ""          "lemma"  | "00000000000019AF"          "lemma"
//!   attribute | "#attr="    "value"  | "#00000000000019AF?attr="   "value"
//!             | "#attr=\""  "value"  | "#00000000000019AF?attr=\"" "value"
//!   zone      | ""          "(zone"  | "(00000000000019AF"         "zone"
//!             | ""          ")zone"  | ")00000000000019AF"         "zone"
struct TKeyLemmaInfo {
    char szLemma[MAXKEY_BUF];
    char szPrefix[MAXKEY_BUF];
    unsigned char Lang;
    TKeyLemmaInfo()
        : Lang(LANG_UNK)
    {
        szLemma[0] = 0;
        szPrefix[0] = 0;
    }
    TKeyLemmaInfo(const TKeyLemmaInfo &a) {
        Copy(a);
    }
    TKeyLemmaInfo &operator=(const TKeyLemmaInfo &a) {
        Copy(a);
        return *this;
    }
    void Copy(const TKeyLemmaInfo &a) {
        strcpy(szLemma, a.szLemma);
        strcpy(szPrefix, a.szPrefix);
        Lang = a.Lang;
    }
    bool IsLiteralAttr() const {
        if (szPrefix[0] == 0)
            return false;
        return szPrefix[strlen(szPrefix) - 1] == '\"';
    }

    inline friend bool operator==(const TKeyLemmaInfo &a, const TKeyLemmaInfo &b) {
        return a.Lang == b.Lang && strcmp(a.szLemma, b.szLemma) == 0 && strcmp(a.szPrefix, b.szPrefix) == 0;
    }
    inline friend bool operator!=(const TKeyLemmaInfo &a, const TKeyLemmaInfo &b) { return !(a == b); }

};


int ConstructKeyWithForms(char *pszRes, int nResBufSize, const TKeyLemmaInfo &lemma, int numForms, const char **pszForms);

//! decodes keys in both formats: intermediate (FAT, keys can be sorted) and storage (key-file) format
//! @note storage format (language in the front of key) will be removed soon, functionality in this function was kept
//!       for a while for backward compatibility
//! @note since key file must contain valid keys this function has minimum checks (see TKeyCodeTest::TestInvalidKeys)
//! @note clients with custom/funny key formats should call DecodeRawKey instead of DecodeKey
//! @todo 0 must be returned for integer attributes (for ex. "#cat=00071102237")
//! @attention minimal size of the buffer that @c pszForms points to is @c N_MAX_FORMS_PER_KISHKA elements
//! @return number of forms in the key or -1 for bad key format
int DecodeKey(const char *pszKey, TKeyLemmaInfo *pLemma, char (*pszForms)[MAXKEY_BUF]);
int DecodeKey(const char *pszKey, TKeyLemmaInfo *pLemma, char (*pszForms)[MAXKEY_BUF], int* formLens);
int DecodeRawKey(const char *pszKey, TKeyLemmaInfo *pLemma);

void ConvertFlagsToStrippedFormat(ui8* flags, ui8* joins, ui8* lang);

/**
 * Converts the given decoded key into stripped format:
 *   - Moves language to forms.
 *   - Removes form joins and titlecase flags.
 */
void ConvertKeyToStrippedFormat(TKeyLemmaInfo *lemma, char (*forms)[MAXKEY_BUF], int* formLens, int formCount);

char *GetLemmaWithPrefix(const char *key, char *lemma_buf);

inline bool NeedRemoveHitDuplicates(const char *pszKey) {
    if (pszKey[0] == OPEN_ZONE_PREFIX || pszKey[0] == CLOSE_ZONE_PREFIX)
        return false;
    return true;
}

/**
 * Appends provided flags and data to the form.
 *
 * @param form                          Pointer to the form.
 * @param[in,out] length                Length of the form.
 * @param bufferLength                  Length of the buffer containing the form.
 *                                      The form (including `\0` terminator) will
 *                                      not grow larger than the provided buffer.
 * @param flags                         Form flags, see `FORM_*` defines in `keychars.h`.
 * @param joins                         Form joins.
 * @param lang                          Form language, member of `ELanguage` enum.
 * @returns                             Whether all the data was successfully appended.
 *                                      A return value of false means that only part of the
 *                                      data was appended as the provided buffer was too small.
 */
bool AppendFormFlags(char* form, int* length, int bufferLength, ui8 flags, ui8 joins, ui8 lang);

/**
 * Removes flags and additional data from the form.
 *
 * @param form                          Pointer to the form.
 * @param[in,out] length                Length of the form.
 * @param[out] flags                    Form flags, see `FORM_*` defines in `keychars.h`.
 * @param[out] joins                    Form joins.
 * @param[out] lang                     Form language, member of `ELanguage` enum.
 */
void RemoveFormFlags(char* form, int* length, ui8* flags, ui8* joins, ui8* lang);

/**
 * Reads flags from the provided form without changing the form.
 *
 * @param form                          Pointer to the form.
 * @param length                        Length of the form.
 * @returns                             Form flags, see `FORM_*` defines in `keychars.h`.
 */
ui8 GetFormFlags(const char* form, int length);

/**
 * Reads language from the provided form without changing the form.
 *
 * @param form                          Pointer to the form.
 * @param length                        Length of the form.
 * @returns                             Form language, member of `ELanguage` enum.
 */
ELanguage GetFormLang(const char* form, int length);

/**
 * Adds given flags to the form.
 *
 * @returns                             Whether the form was changed.
 */
bool EncaseWord(char *form, int length, int bufferLength, ui8 flags);

/**
 * Removes flags from the given form.
 *
 * @returns                             Whether the form was marked with
 *                                      FORM_TITLECASE flag.
 */
bool DecaseWord(char *form, int length);


//! makes uppercase the first char of a UTF8 encoded word
//! @param word     buffer containing word, the length of the buffer must be at least MAXKEY_BUF
//! @note 'word' must be null-terminated: word[len] == 0;
//!       in some cases length of the word can be changed, for example
//!       0x131 'latin small letter dotless i' is converted to ASCII 0x49 'I',
//!       probably there can be cases when length is increased
//! @throw yexception - conversion from UTF8 failed,
//!        yexception - buffer overflow because of uppercase conversion
void UpperCaseFirstChar(char* word, size_t len);


//! interprets a key of index
class TKeyReader : private TNonCopyable {
public:
    struct TForm {
        const char* Text;
        ui8 Flags;
        ui8 Joins;
        ui8 Lang;
    };

private:
    TKeyLemmaInfo Lemma;
    char FormBuf[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];   //!< @c DecodeKey requires that buffer contains minimum @c N_MAX_FORMS_PER_KISHKA forms
    TForm Forms[N_MAX_FORMS_PER_KISHKA];                //!< processed forms, initially their members filled with zeros
    size_t FormCount;

public:
    explicit TKeyReader(const char* key, bool rawKey = false) {
        memset(Forms, 0, sizeof(Forms));
        const int i = (rawKey ? DecodeRawKey(key, &Lemma) : DecodeKey(key, &Lemma, FormBuf));
        FormCount = i > 0 ? (size_t)i : 0;
    }
    const char* GetPrefix() const {
        return Lemma.szPrefix;
    }
    //! returns @b true if lemma is UTF8 encoded
    bool IsUTF8() const {
        return Lemma.szLemma[0] == UTF8_FIRST_CHAR;
    }
    const char* GetLemma() const {
        return (IsUTF8() ? Lemma.szLemma + 1 : Lemma.szLemma);
    }
    const char* GetRawLemma() const {
        return Lemma.szLemma;
    }
    size_t GetFormCount() const {
        return FormCount;
    }
    ELanguage GetLang() const {
        return (ELanguage)Lemma.Lang;
    }
    const TForm& GetForm(size_t i);

    bool FormIsUTF8(size_t i) const {
        return FormBuf[i][0] == UTF8_FIRST_CHAR;
    }
};

void AppendLanguage(char* buffer, ELanguage language);
void AppendLanguage(char* buffer, const TKeyReader& reader);
void PrintLemmaWithForms(char* buffer, TKeyReader& reader, bool printLang);

inline int EncodePrefix(ui64 prefix, char* out) {
    unsigned char* pstart = (unsigned char*)&prefix;
    unsigned char* p = pstart + sizeof(prefix) - 1; // last byte of prefix
    char* o = out;
    while (p >= pstart) {
        unsigned char c = *p--;
        *o++ = DigitToChar(c / 16);
        *o++ = DigitToChar(c % 16);
    }
    *o = 0;
    return int(o - out);
}

inline ui64 DecodePrefix(const char* key) {
    Y_ASSERT(key && key[0]);
    if (key[0] == ATTR_PREFIX || key[0] == OPEN_ZONE_PREFIX || key[0] == CLOSE_ZONE_PREFIX) {
        ++key;
    }

    ui64 prefix = 0;
    for (size_t i = 0; i < 2 * sizeof(prefix); ++i) {
        Y_ASSERT(key[i]);
        prefix *= 16;
        prefix += Char2Digit(key[i]);
    }
    return prefix;
}

//! the buffer must have length at least PUNCT_PREFIX_BUF characters
template <typename T>
inline void EncodeWordPrefix(wchar16 prefix, T* buffer) {
    Y_ASSERT(prefix == '@' || prefix == '#' || prefix == '$');
    buffer[0] = PUNCT_PREFIX;
    buffer[1] = DigitToChar(prefix >> 4);
    buffer[2] = DigitToChar(prefix & 15);
    buffer[3] = 0;
}

inline char DecodeWordPrefix(const wchar16* p, size_t n) {
    Y_ASSERT(n == PUNCT_PREFIX_LEN && p[0] == PUNCT_PREFIX && isxdigit(p[1]) && isxdigit(p[2]));
    Y_UNUSED(n);
    return (char)((Char2Digit(p[1]) << 4) | Char2Digit(p[2]));
}

