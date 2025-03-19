#include "index_format_usage.h"

#include <util/generic/hash.h>

namespace NDoom {

TString GetIndexFormatUsage(EIndexFormat indexFormat) {
    static const THashMap<EIndexFormat, TString> USAGE = {
        { YandexIndexFormat, "old key/inv format" },
        { YandexCountsIndexFormat, "format of intermediate index used to build panther index " },
        { YandexPantherIndexFormat, "panther index format sorted" },
        { Array4dIndexFormat, "Old 4D array format used in factorann" },
        { OffroadCountsIndexFormat, "offroad-compressed format for counts (interim panther) hits." },
        { OffroadPantherIndexFormat, "offroad-compressed format for panther hits." },
        { OffroadDoublePantherIndexFormat, "offroad-compressed format for double-panther hits (see offroad_double_panther_wad)." },
        { OffroadAnnDataWadFormat, "offroad-compressed format for indexfactorann.data.wad." },
        { OffroadFastAnnDataWadFormat, "offroad-compressed format for indexfactorann.data.wad." },
        { OffroadAnnWadSortedMultiKeysFormat, "offroad-compressed format with sorted multi keys for indexann.wad." },
        { OmniIndexFormat, "format of index.omni in web search" },
        { OmniVideoFormat, "format of index.omni in video search" },
        { DocAttrs64Format, "format of index.aa with 64-bit attributes" },
        { ErfFormat, "format of indexerf2" },
        { RegErfFormat, "format of indexregerf" },
        { RegHostErfFormat, "format of indexregherf" },
        { HostErfFormat, "format of indexherf" },
        { SentFormat, "format of indexsent" },
        { AnnSentFormat, "format of indexann.sent" },
        { LinkAnnSentFormat, "format of indexlinkann.sent" },
        { HnswFormat, "format of indexhnsw" },
        { FactorAnnSentFormat, "format of indexfactorann.sent" },
        { MegaWadFormat, "format of unified wad, containing any indices" },
        { IndexAttrsFormat, "format of indexattrs" },
        { FrqFormat, "format of indexfrq" },
        { DocAttrsFormat, "format of indexaa" },
        { CategToNameFormat, "format of c2n" },
        { InvHashFormat, "format of indexinvhash" },
        { HashedKeyInvFormat, "format of index with offroad_hashed_keyinv" },
        { VideoErfFormat, "format of video indexerf2" },
        { VideoRegErfFormat, "format of video indexregerf" },
        { VideoHostRegErfFormat, "format of video indexregherf" },
        { VideoAuthorRegErfFormat, "format of video indexautherf" },
        { VideoTopRegErfFormat, "format of video indextoperf" },
        { WebItdItpSlimIndexFormat, "format of itditp indexitditpslim.wad" },
    };

    Y_ENSURE(indexFormat != UnknownIndexFormat);
    return USAGE.at(indexFormat);
}

} // namespace NDoom
