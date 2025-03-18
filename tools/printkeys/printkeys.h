#pragma once

#include <kernel/search_types/search_types.h>
#include <util/generic/maybe.h>
#include <kernel/keyinv/indexfile/indexstorageface.h>

enum PRINTPOS_TYPE {
   PRINTPOS_NONE,
   PRINTPOS_WORDPOS,
   PRINTPOS_WORDPOS_NFORM,
   PRINTPOS_DOCID,
   PRINTPOS_DOCLENG,
   PRINTPOS_HEX
};

struct PRINTKEYS_OPTIONS
{
    bool print_valid_only;
    IYndexStorage::FORMAT is_full_format;
    PRINTPOS_TYPE print_pos;

    bool print_offset;
    bool print_exact_keys;
    bool use_mapping;
    char key_prefix[MAXKEY_BUF];
    bool collect_stats;
    bool use_tab;
    bool print_lang;
    bool remove_prefix;
    bool print_hitlist;
    ui32 hit_count;
    ui32 hit_length;
    ui64 hit_offset;
    TMaybe<ui32> doc_id;
};

enum PRINTKEYS_TYPE {
   PRINTKEYS_ALL,
   PRINTKEYS_CHECKED_ONLY,
   PRINTKEYS_BASTARD_ONLY
};

struct CHECKKEYS_OPTIONS
{
    PRINTKEYS_TYPE how_to_print;
};

inline bool is_valid_key(const char* str){
    while (*str){
        if (*str == '\n' || *str == '\t')
            return false;
        str++;
    }
    return true;
}
