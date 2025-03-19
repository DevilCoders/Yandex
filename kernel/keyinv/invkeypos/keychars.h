#pragma once

#include <util/generic/flags.h>

#define TITLECASECOLLATOR ((char)('\x01'))
#define UTF8_FIRST_CHAR   ((char)127)
#define LEMMA_DELIM       ((char)'\x01')    // delimits lemma and forms:    "lemma \x01 forms \x00"
#define LEMMA_LANG_DELIM  ((char)'\x02')    // delimits lemma and language: "lemma \x02 lang \x01 forms \x00", "lemma \x02 lang \x00"
#define LEMMA_LANG_PREFIX ((char)'?')       // language prefix of lemma in key: "? lang lemma \x01 forms \x00 counter length"


// Flags byte of the form

enum EFormFlag: unsigned char {
    //! Form starts with a capital letter, e.g. "Word".
    FORM_TITLECASE = 0x01,

    //! Form is a part of a multiword expression, e.g. "mp3" ("mp" and "3" are separate forms) or "long-established".
    FORM_HAS_JOINS = 0x02,

    //! Form has associated language.
    FORM_HAS_LANG = 0x04,

    //! Form was transliterated. Currently used only in URLs.
    //! Note that the word itself will appear in the form in source (e.g. Russian) language, not in English.
    FORM_TRANSLIT = 0x08,

    //! Max value for flags/joins byte. Must not define a printable UTF8/YANDEX symbol.
    FORM_FLAGS_MASK = 0x0F,

    FORM_VERSION_MASK = 0xF0,
    FORM_V1_ENCODED = 0x00,
    FORM_V2_ENCODED = 0x10,

    FORM_VALID_CHAR_MASK = 0xE0
};

Y_DECLARE_FLAGS(EFormFlags, EFormFlag);
Y_DECLARE_OPERATORS_FOR_FLAGS(EFormFlags);


// Joins byte of the form

//! Form has another word to the left, e.g. in "mp3" this flag would be set for "3".
#define FORM_LEFT_JOIN    ((ui8)0x01)

//! Form has another word to the left that is separated from it with a delimiter.
#define FORM_LEFT_DELIM   ((ui8)0x02)

//! Form has another word to the right.
#define FORM_RIGHT_JOIN   ((ui8)0x04)

//! Form has another word to the right that is separated from it with a delimiter.
//! It can be changed to mask (0x0C) if right delimiter will have length greater than 1 (?).
#define FORM_RIGHT_DELIM  ((ui8)0x08)


#define INFO_BYTE_FORMS_BIT ((ui8)0x01)     // forms flag AKA LEMMA_DELIM
#define INFO_BYTE_LANG_BIT  ((ui8)0x02)     // see also LEMMA_LANG_DELIM
#define INFO_BYTE_MAX_VALUE ((ui8)0x0F)     // max value of the byte at the end of lemma actually it should be max(0x02, 0x01) (lang OR forms)
                                            // but temporary it used for reading old keys that have lemmas with joins

#define OPEN_ZONE_PREFIX  '('
#define CLOSE_ZONE_PREFIX ')'
#define ATTR_PREFIX       '#'
#define PUNCT_PREFIX      '%'
#define KEY_PREFIX_DELIM      '?'

#define PUNCT_PREFIX_LEN    3
#define PUNCT_PREFIX_BUF    (PUNCT_PREFIX_LEN + 1)

//! join type of word in index
//! @attention the first and second bits (0x01, 0x02) cannot be used
enum EJoinType {
    JOIN_NONE = 0x00,
    JOIN_LEFT = 0x04,
    JOIN_RIGHT = 0x08,
    JOIN_BOTH = JOIN_LEFT | JOIN_RIGHT
};
