#include <cassert>
#include <algorithm>

#include <kernel/search_types/search_types.h>  // MAXKEY_LEN, MAXKEY_BUF
#include <library/cpp/charset/recyr.hh>

#include <util/generic/string.h>
#include <util/charset/unidata.h>
#include <util/charset/utf8.h>

#include "keycode.h"

static inline char* add(char *psz, const char *p, const char *pszEnd) {
    while (psz < pszEnd && *p)
        *psz++ = *p++;
    return psz;
}

static inline void AddInfoBytes(char*& p, unsigned char lang, const char* end) {
    if (lang != LANG_UNK) {
        if ((p + 2) <= end) { // (p + 2) can be equal to end after storing the language
            *p++ = LEMMA_LANG_DELIM;
            *p++ = lang; // language ID
        }
    }
}

//! copies lemma that can have key prefix
static inline char* CopyLemma(char* dst, const unsigned char*& p, unsigned char stopChar, TKeyLemmaInfo* lemma) {
   while (*p > stopChar) {
        if (*p == KEY_PREFIX_DELIM && p[1] > stopChar) {
            *dst = 0;
            ++p;
            strcpy(lemma->szPrefix, lemma->szLemma);
            dst = lemma->szLemma;
            while (*p > stopChar)
                *dst++ = *p++;
            break;
        }
        *dst++ = *p++;
    }
    return dst;
}

static bool CanUseSingleFormCheat(const char *pszKey) {
    if (!pszKey || pszKey[0] == 0)
        return false;
    if (pszKey[0] == '!' && pszKey[1] == '!')
        return false;
    if (pszKey[0] == '#' && pszKey[1] == '#')
        return false;
    return true;
}

//! @attention pszRes should be at least MAXKEY_BUF length
/**
    key can have the following content:
    no forms:
        1. lemma + 0
        2. lemma + '\x01' + 0                (prev. version key with first capital letter of the lemma)
        3. lemma + '\x02' + lang + 0
    with forms:
        4. lemma + '\x01' + forms + 0
        5. lemma + '\x02' + lang + '\x01' + forms + 0

    no forms - with join:
        6. lemma + '\x04' + 0           join left
           lemma + '\x08' + 0           join right
           lemma + '\x0C' + 0           join both

        7. lemma + '\x06' + lang + 0           join left + lang
           lemma + '\x0A' + lang + 0           join right + lang
           lemma + '\x0E' + lang + 0           join both + lang

    with forms and join:
        8. lemma + '\x04' + '\x01' + forms + 0          join left + forms
           lemma + '\x08' + '\x01' + forms + 0          join right + forms
           lemma + '\x0C' + '\x01' + forms + 0          join both + forms

        9. lemma + '\x07' + lang + '\x01' + forms + 0          join left + lang + forms
           lemma + '\x0B' + lang + '\x01' + forms + 0          join right + lang + forms
           lemma + '\x0F' + lang + '\x01' + forms + 0          join both + lang + forms

    *- key format is compatible with old format (without joins)
*/
//! @note pszRes should be at least MAXKEY_BUF length
int ConstructKeyWithForms(char *pszRes, int nResBufSize,
    const TKeyLemmaInfo &lemma, int numForms, const char **pszForms)
{
    if (nResBufSize < MAXKEY_BUF) {
        assert(0);
        *pszRes = 0;
        return 0;
    }

    char *pszStart = pszRes;
    char *pszEnd = pszRes + nResBufSize - 1;
    if ((void *)lemma.szPrefix && *lemma.szPrefix) {
        pszRes = add(pszRes, lemma.szPrefix, pszEnd);
        if (*(pszRes-1) != '=') { // not attribute
            *pszRes++ = KEY_PREFIX_DELIM;
        }
    }
    assert(*lemma.szLemma);
    pszRes = add(pszRes, lemma.szLemma, pszEnd);
    assert(numForms != 0 || !CanUseSingleFormCheat(pszStart)); // CanUseSingleFormCheat -> numForms != 0

    if (numForms == 0 || (numForms == 1 && strcmp(lemma.szLemma, pszForms[0]) == 0 && CanUseSingleFormCheat(pszStart))) {
        AddInfoBytes(pszRes, lemma.Lang, pszEnd);
        *pszRes++ = 0;
        return int(pszRes - pszStart);
    }

    AddInfoBytes(pszRes, lemma.Lang, pszEnd);
    pszRes = add(pszRes, "\x01", pszEnd); // in TYndexFile::WriteKey() join, lang flag and forms delim stored to the same byte

    const char *pszPrev = lemma.szLemma;
    for (int i = 0; i < numForms; ++i) {
        int nSame = 0, nDifferent = 0;
        const char *pszPack = pszForms[i];
        for (; pszPack[nSame] && pszPack[nSame] == pszPrev[nSame];)
            ++nSame;
        nDifferent = (int)strlen(pszPack + nSame);
        if ((nSame < 16 && nDifferent < 8) && (nSame != 0 || nDifferent != 0)) {
            unsigned char n = (unsigned char)((nDifferent << 4) | nSame);
            *pszRes++ = n;
            pszRes = add(pszRes, pszPack + nSame, pszEnd);
        } else {
            nDifferent += nSame;
            nSame = Min(nSame, 127);
            nDifferent -= nSame;
            *pszRes++ = (unsigned char)(nSame|0x80);
            assert(nDifferent < 255);
            *pszRes++ = (unsigned char)(nDifferent + 1);
            pszRes = add(pszRes, pszPack + nSame, pszEnd);
        }
        pszPrev = pszForms[i];

        if (pszRes >= pszEnd) {
            // buffer overflow
            pszEnd[0] = 0;
            return nResBufSize;
        }
    }
    *pszRes++ = 0;
    int nResLength = int(pszRes - pszStart);
    assert(nResLength < nResBufSize);
    return nResLength;
}

int DecodeRawKey(const char *pszKey, TKeyLemmaInfo *pLemma) {
    pLemma->Lang = LANG_UNK;
    *pLemma->szPrefix = 0;
    strcpy(pLemma->szLemma, pszKey);
    return 0;
}

int DecodeKey(const char *pszKey, TKeyLemmaInfo *pLemma, char (*pszForms)[MAXKEY_BUF]) {
    int lens[N_MAX_FORMS_PER_KISHKA];
    return DecodeKey(pszKey, pLemma, pszForms, lens);
}

int DecodeKey(const char *pszKey, TKeyLemmaInfo *pLemma, char (*pszForms)[MAXKEY_BUF], int* formLens) {
    for (unsigned i = 0; i < N_MAX_FORMS_PER_KISHKA; ++i) {
        pszForms[i][0] = 0;
        formLens[i] = 0;
    }

    const unsigned char *psz = reinterpret_cast<const unsigned char*>(pszKey);

    //! 1) lemma x02 L x01 forms x00
    //! 2) lemma x02 L x00
    //! ----------------------------------------
    //! 3) lemma x01 forms x00
    //! 4) lemma x00

    if (*psz == '?') { // language before lemma - old format of key: '?' lang lemma x01 forms x00
        ++psz;
        if (*psz)
            pLemma->Lang = *psz++; // language ID
        pLemma->szPrefix[0] = 0;
    } else if (*psz == ATTR_PREFIX) { // extract prefix (#cat=) or (#00000000000001AF?url=")
        pLemma->szLemma[0] = 0;
        if (psz[1] == ATTR_PREFIX) { // (##_DOC_IDF_SUM)
            strcpy(pLemma->szPrefix, reinterpret_cast<const char*>(psz));
            return 0;
        }
        pLemma->Lang = LANG_UNK;
        char *szPrefixDst = pLemma->szPrefix;
        while (*psz && *psz != '=')
            *szPrefixDst++ = *psz++;
        if (*psz)
            *szPrefixDst++ = *psz++;
        if (*psz == '\"') {
            // literal attribute
            *szPrefixDst++ = *psz++;
        }
        *szPrefixDst = 0;
        if (*psz == 0)
            return -1; // (#url=") or (#cat=) no value
        strcpy(pLemma->szLemma, reinterpret_cast<const char*>(psz));
        return 0;
    } else if (*psz == OPEN_ZONE_PREFIX || *psz == CLOSE_ZONE_PREFIX) {
        pLemma->Lang = LANG_UNK;
        pLemma->szPrefix[0] = 0;
        strcpy(pLemma->szLemma, reinterpret_cast<const char*>(psz));
        return 0;
    } else {
        pLemma->Lang = LANG_UNK;
        pLemma->szPrefix[0] = 0;
    }

    // extract lemma
    {
        char *szLemmaDst = CopyLemma(pLemma->szLemma, psz, INFO_BYTE_MAX_VALUE, pLemma);
        if (psz[0] == LEMMA_DELIM && psz[1] == 0) {
            // looks like prev version key with first capital letter, copy 0x01 to output
            *szLemmaDst++ = *psz++;
        }
        *szLemmaDst = 0;
    }

    // interpret the info byte
    // @attention hasForms is '\0' in case of: lemma x02 L x01 forms x00
    const unsigned char hasForms = (*psz & INFO_BYTE_FORMS_BIT); // forms flag just after lemma in case of storage format
    const unsigned char langFlag = (*psz & INFO_BYTE_LANG_BIT);  // language after lemma in case of intermediate format

    // @see ConstructKeyWithForms for comments
    if (langFlag) {
        ++psz;
        pLemma->Lang = *psz; // language ID
        Y_ASSERT(pLemma->Lang != LANG_UNK);
        ++psz;
    } else if (hasForms)
        ++psz;

    // extract forms if there are any
    if (!hasForms) {
        if (*psz != TITLECASECOLLATOR) {
            if (CanUseSingleFormCheat(pszKey)) {
                if (pLemma->szLemma[0] == 0)
                    return -1;
                // assume optimization for lemma = form case was used
                strcpy(pszForms[0], pLemma->szLemma);
                formLens[0] = (int)strlen(pszForms[0]);
                return 1;
            }
            return 0;
        }
        ++psz;
    }

    char convertToV2 = 0;
    unsigned nForm = 0;
    const char *pszPrev = pLemma->szLemma;
    while (*psz && nForm < N_MAX_FORMS_PER_KISHKA) {
        unsigned char n = *psz++;
        int nSame, nDifferent;
        if (n & 0x80) {
            nSame = n & 0x7f;
            n = *psz++;
            nDifferent = n - 1;
        } else {
            nSame = n & 15;
            nDifferent = (n >> 4) & 7;
        }
        char *pszDst = pszForms[nForm];
        if (nSame + nDifferent >= MAXKEY_BUF) {
            pszDst[0] = 0;
            return -1;
        } else {
            memcpy(pszDst, pszPrev, nSame);
            pszDst += nSame;
            for (const unsigned char* pszEnd = psz + nDifferent; psz < pszEnd;) {
                const char c = *psz++;
                *pszDst++ = c;
                if (c == 0)
                    return -1;
            }
            convertToV2 |= (*(pszDst - 1) & FORM_VERSION_MASK) == FORM_V1_ENCODED;

            *pszDst = 0;
            formLens[nForm] = int(pszDst - pszForms[nForm]);
        }

        pszPrev = pszForms[nForm];
        ++nForm;
    }

    if (*psz)
        return -1;

    /* Convert forms to new format if needed. This will rarely happen. */
    if (convertToV2) {
        ui8 flags = 0, joins = 0, lang = LANG_UNK;
        for (unsigned i = 0; i < nForm; i++) {
            RemoveFormFlags(pszForms[i], &formLens[i], &flags, &joins, &lang);
            AppendFormFlags(pszForms[i], &formLens[i], MAXKEY_BUF, flags, joins, lang);
        }
    }

    return nForm;
}

void ConvertFlagsToStrippedFormat(ui8* flags, ui8* joins, ui8* lang) {
    /* Drop joins & titlecase. */
    *joins = 0;
    *flags &= FORM_TRANSLIT;

    /* Handle language properly. */
    if (*lang != LANG_UNK)
        *flags |= FORM_HAS_LANG;
}

void ConvertKeyToStrippedFormat(TKeyLemmaInfo *lemma, char (*forms)[MAXKEY_BUF], int* formLens, int formCount) {
    if (formCount == 0)
        return; /* Don't strip language in this case... */

    ui8 Lang = lemma->Lang;
    lemma->Lang = LANG_UNK;

    for (int i = 0; i < formCount; i++) {
        ui8 formFlags, formJoins, formLang;
        RemoveFormFlags(forms[i], &formLens[i], &formFlags, &formJoins, &formLang);

        if (formLang == LANG_UNK)
            formLang = Lang;
        ConvertFlagsToStrippedFormat(&formFlags, &formJoins, &formLang);

        AppendFormFlags(forms[i], &formLens[i], MAXKEY_BUF, formFlags, formJoins, formLang);
    }
}

char *GetLemmaWithPrefix(const char *key, char *lemma_buf) {
    // extract lemma
    char *dst = lemma_buf;
    while (*key && *key != TITLECASECOLLATOR)
        *dst++ = *key++;
    if (key[0] == TITLECASECOLLATOR && key[1] == 0) {
        // looks like prev version key with first capital letter, copy 0x01 to output
        *dst++ = *key++;
    }
    *dst = 0;
    return lemma_buf;
}

void UpperCaseFirstChar(char* word, size_t len) {
    Y_ASSERT(word && len);

    unsigned char *const b = reinterpret_cast<unsigned char*>(word);
    unsigned char *const e = b + len;
    wchar32 c = 0;
    size_t n = 0;

    RECODE_RESULT r = SafeReadUTF8Char(c, n, b, e);
    if (r != RECODE_OK)
        ythrow yexception() << "conversion from UTF8 failed";

    wchar32 uc = ToUpper(c);
    const size_t bufSize = 4;
    unsigned char buf[bufSize];
    size_t un = 0;
    r = SafeWriteUTF8Char(uc, un, buf, buf + bufSize);
    Y_ASSERT(r == RECODE_OK);

    // in this case length of the result word can be greater than 'len'
    if (n != un) {
        const size_t tail = len - n;
        // new length must be less than length of buffer in opposite case an exception is thrown
        // when turkish characters 'latin capital letter I with dot above' 0x131 and 'latin small letter dotless i' 0x130
        // is converted there always exists truncated flag x01 so buffer will always has enough space,
        // moreover there is skipped UTF8_FIRST_CHAR that can be used as well...
        // only if length of character is increased from 1 to 4 buffer in theory can have no enough space but
        // this can take place only if ToUpper() will take language as parameter
        if (un + tail >= size_t(MAXKEY_BUF - 1))
            ythrow yexception() << "buffer overflow because of uppercase conversion";
        memmove(b + un, b + n, tail + 1); // move tail of word with null-terminator
    }
    memcpy(b, buf, un);
}

const TKeyReader::TForm& TKeyReader::GetForm(size_t i) {
    Y_ASSERT(i < FormCount);
    if (!Forms[i].Text) {
        char* form = FormBuf[i];
        int len = (int)strlen(form);
        if (len) {
            RemoveFormFlags(form, &len, &Forms[i].Flags, &Forms[i].Joins, &Forms[i].Lang);
            if (form[0] == UTF8_FIRST_CHAR) {
                ++form;
                --len;
                if (Forms[i].Flags & FORM_TITLECASE)
                    UpperCaseFirstChar(form, len);
            } else {
                if (Forms[i].Flags & FORM_TITLECASE)
                    form[0] = csYandex.ToUpper(form[0]);
            }
        }
        Forms[i].Text = form;
    }
    return Forms[i];
}

/**
 * Compatibility with old form format: form [flags]? [joins]?
 */
void GetFormFlagsV1(const char* form, int* length, ui8* flags, ui8* joins) {
    /* In very old databases flags were different.
     * This is why we fix whatever inconsistencies we encounter. */

    if ((ui8)form[*length - 2] <= FORM_FLAGS_MASK) {
        /* Has joins. */
        *joins = form[--*length];
        *flags = form[--*length] & (FORM_TITLECASE | FORM_TRANSLIT | (*joins == 0 ? 0 : FORM_HAS_JOINS));
    } else {
        /* Has flags only. */
        *joins = 0;
        *flags = form[--*length] & (FORM_TITLECASE | FORM_TRANSLIT);
    }
}

/**
 * New sane format.
 * Form [lang]? [joins]? [flags]?
 *
 * Upper four bits of the joins byte are unused right now, and that's a place
 * for extension.
 */
void GetFormFlagsV2(const char* form, int* length, ui8* flags, ui8* joins, ui8* lang) {
    *flags = form[--*length] & FORM_FLAGS_MASK;

    if (*flags & FORM_HAS_JOINS) {
        *joins = form[--*length];
    } else {
        *joins = 0;
    }

    if (*flags & FORM_HAS_LANG) {
        *lang = form[--*length];
    } else {
        *lang = LANG_UNK;
    }
}

void RemoveFormFlags(char* form, int* length, ui8* flags, ui8* joins, ui8* lang) {
    Y_ASSERT(form && *length > 0);

    if (*length <= 1) {
        *flags = 0;
        *joins = 0;
        *lang = LANG_UNK;
        return;
    }

    ui8 version = form[*length - 1] & FORM_VERSION_MASK;
    if (version == FORM_V2_ENCODED) {
        GetFormFlagsV2(form, length, flags, joins, lang);
        form[*length] = '\0';
    } else if (version == FORM_V1_ENCODED) {
        *lang = LANG_UNK;
        GetFormFlagsV1(form, length, flags, joins);
        form[*length] = '\0';
    } else {
        *flags = 0;
        *joins = 0;
        *lang = LANG_UNK;
    }

}

ui8 GetFormFlags(const char* form, int length) {
    Y_ASSERT(form && length > 0);

    ui8 version = form[length - 1] & FORM_VERSION_MASK;
    if (version == FORM_V2_ENCODED) {
        return form[length - 1] & FORM_FLAGS_MASK;
    } else if (version == FORM_V1_ENCODED) {
        /* Do it the slow(er) way as we don't want too much copypasta here. */
        ui8 flags, joins;
        GetFormFlagsV1(form, &length, &flags, &joins);
        return flags;
    } else {
        /* No flags, form ends with a valid character. */
        return 0;
    }
}

ELanguage GetFormLang(const char* form, int length) {
    Y_ASSERT(form && length > 0);

    if (length <= 1)
        return LANG_UNK;

    ui8 version = form[length - 1] & FORM_VERSION_MASK;
    if (version == FORM_V2_ENCODED) {
        int len = length;
        ui8 flags = 0, joins = 0, lang = 0;
        GetFormFlagsV2(form, &len, &flags, &joins, &lang);
        return static_cast<ELanguage>(lang);
    } else
        return LANG_UNK;
}

bool AppendFormFlags(char* form, int* length, int bufferLength, ui8 flags, ui8 joins, ui8 lang) {
    Y_ASSERT(flags <= FORM_FLAGS_MASK);
    Y_ASSERT((lang != LANG_UNK) == !!(flags & FORM_HAS_LANG));
    Y_ASSERT((joins != 0) == !!(flags & FORM_HAS_JOINS));

    bool result = true;
    if (lang != LANG_UNK) {
        if (*length < bufferLength - 2) {
            form[*length] = lang;
            ++*length;
        } else {
            flags &= ~FORM_HAS_LANG;
            result = false;
        }
    }

    if (joins != 0) {
        if (*length < bufferLength - 2) {
            form[*length] = joins;
            ++*length;
        } else {
            flags &= ~FORM_HAS_JOINS;
            result = false;
        }
    }

    if (flags != 0) {
        if (*length < bufferLength - 1) {
            form[*length] = flags | FORM_V2_ENCODED;
            ++*length;
        } else {
            result = false;
        }
    }

    form[*length] = 0;
    return result;
}

bool EncaseWord(char *form, int length, int bufferLength, ui8 flags) {
    flags = flags & (FORM_TITLECASE | FORM_TRANSLIT); /* Other flags not allowed as they require more data. */
    if (!flags)
        return false;

    AppendFormFlags(form, &length, bufferLength, flags, 0, LANG_UNK);
    return true;
}

bool DecaseWord(char *form, int length) {
    ui8 flags, joins, lang;

    RemoveFormFlags(form, &length, &flags, &joins, &lang);

    return flags & FORM_TITLECASE;
}

void AppendLanguage(char* buffer, ELanguage language) {
    strcat(buffer, " {"); // space required here because in FormatKey it will be compared with opt.key_prefix
    strcat(buffer, IsoNameByLanguage(language));
    strcat(buffer, "}");
}

void AppendLanguage(char* buffer, const TKeyReader& reader) {
    AppendLanguage(buffer, reader.GetLang());
}

static void AppendFormJoins(char* buffer, const TKeyReader::TForm& form) {
    size_t i = strlen(buffer);
    buffer[i++] = ' ';
    if (form.Joins & FORM_LEFT_JOIN) {
        buffer[i++] = 'L';
        if (form.Joins & FORM_LEFT_DELIM)
            buffer[i++] = '+';
    }
    if (form.Joins & FORM_RIGHT_JOIN) {
        buffer[i++] = 'R';
        if (form.Joins & FORM_RIGHT_DELIM)
            buffer[i++] = '+';
    }
    buffer[i] = 0;
}

void PrintLemmaWithForms(char* buffer, TKeyReader& reader, bool printLang) {
    strcpy(buffer, reader.GetPrefix());
    strcat(buffer, reader.IsUTF8() ? reader.GetLemma() : RecodeFromYandex(CODES_UTF8, reader.GetLemma()).data());
    if (printLang && reader.GetLang() != LANG_UNK)
        AppendLanguage(buffer, reader);
    strcat(buffer, " (");
    const size_t n = reader.GetFormCount();
    for (size_t i = 0; i < n; ++i) {
        TKeyReader::TForm form = reader.GetForm(i);
        strcat(buffer, reader.FormIsUTF8(i) ? form.Text : RecodeFromYandex(CODES_UTF8, form.Text).data());

        if (form.Joins)
            AppendFormJoins(buffer, form);
        if (form.Lang)
            AppendLanguage(buffer, static_cast<ELanguage>(form.Lang));

        strcat(buffer, (i == n - 1 ? ")" : ", "));
    }
}

