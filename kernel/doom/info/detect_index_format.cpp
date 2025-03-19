#include "detect_index_format.h"

#include <array>

#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/system/fs.h>
#include <util/folder/path.h>

#include "index_info.h"

namespace {

/* Copy from array4d_poly.cpp */
const char ARRAY_4D_POLY_MAGIC_VALUE[] = "Array4DPoly\0\0\0\0";
const char OMNI_INDEX_MAGIC_VALUE[] = "jsstruct";
const char MEGAWAD_MAGIC_VALUE[] = "IWADMWAD";

static_assert(sizeof(ARRAY_4D_POLY_MAGIC_VALUE) == 16, "Expecting array4d header magic size == 16.");

}

namespace NDoom {

EIndexFormat DetectIndexFormat(const TString& indexPath) {
    EIndexFormat indexFormat = UnknownIndexFormat;

    /* Try .info file first. */
    TIndexInfo info = LoadIndexInfo(indexPath);
    if (info.HasFormat()) {
        indexFormat = FromString<EIndexFormat>(info.GetFormat());
        if (indexFormat != UnknownIndexFormat) {
            return indexFormat;
        }
    }
    TFsPath path(indexPath);
    if (path.GetName() == TStringBuf("indexerf2")) {
        return ErfFormat;
    }
    if (path.GetName() == TStringBuf("indexregherf")) {
        return RegErfFormat;
    }
    if (path.GetName() == TStringBuf("indexattrs.")) {
        return IndexAttrsFormat;
    }
    if (path.GetName() == TStringBuf("indexhsnw")) {
        return HnswFormat;
    }
    if (path.GetName() == TStringBuf("indexitditpslim.wad")) {
        return WebItdItpSlimIndexFormat;
    }

    /* Try to read magic from magic-based formats. */
    try {
        std::array<char, 16> header;
        TIFStream input(indexPath);
        size_t size = input.Load(header.data(), 16);

        if (size == 16 && memcmp(header.data(), ARRAY_4D_POLY_MAGIC_VALUE, 16) == 0)
            return Array4dIndexFormat;

        if (size >= 8 && memcmp(header.data(), OMNI_INDEX_MAGIC_VALUE, 8) == 0)
            return OmniIndexFormat;

        if (size >= 8 && memcmp(header.data(), MEGAWAD_MAGIC_VALUE, 8) == 0)
            return MegaWadFormat;
    } catch (...) {
        /* No file there? Just go on. */
    }

    /* Maybe it's just a keyinv? */
    TString keyPath = indexPath + "key";
    TString invPath = indexPath + "inv";
    if (NFs::Exists(keyPath) && NFs::Exists(invPath)) {
        if (indexPath.Contains("indexpanther")) {
            return YandexPantherIndexFormat;
        } else if (indexPath.Contains("indexcounts")) {
            return YandexCountsIndexFormat;
        } else {
            return YandexIndexFormat;
        }
    }

    return UnknownIndexFormat;
}

} // namespace NDoom
