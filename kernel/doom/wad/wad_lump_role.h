#pragma once

namespace NDoom {

// TODO: make it a normal enum

/**
 * Role of a lump that is part of a WAD-based index.
 *
 * Answers to the question "what role does this lump serve for the given index".
 *
 * Please don't add separate roles for different storage / encoding formats
 * of the same data. Use info lump for that.
 */
enum class EWadLumpRole {
    Hits        /* "hits" */,
    HitSub      /* "hits_sub" */,
    HitsModel   /* "hits_model" */,

    Keys        /* "keys" */,
    KeyFat      /* "keys_fat" */,
    KeyIdx      /* "keys_idx" */,
    KeysModel   /* "keys_model" */,

    Struct          /* "struct" */,
    StructSub       /* "struct_sub" */,
    StructSize      /* "struct_size" */,
    StructModel     /* "struct_model" */,
};

} // namespace NDoom
