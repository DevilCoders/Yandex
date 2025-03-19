#pragma once

namespace NDoom {

enum EIndexFormat {
    UnknownIndexFormat = 0              /* "unknown" */,

    YandexIndexFormat                   /* "yandex" */,
    YandexCountsIndexFormat             /* "yandex_counts" */, // TODO: DROP!
    YandexPantherIndexFormat            /* "yandex_panther" */, // TODO: DROP!

    Array4dIndexFormat                  /* "array4d" */,

    OffroadCountsIndexFormat            /* "offroad_counts" */,
    OffroadPantherIndexFormat           /* "offroad_panther" */,
    OffroadDoublePantherIndexFormat     /* "offroad_double_panther" */,

    // TODO: only used as a wrangler arg, need to drop these:
    OffroadAnnDataWadFormat             /* "offroad_ann_data_wad" */,
    OffroadFastAnnDataWadFormat         /* "offroad_fast_ann_data_wad" */,
    OffroadAnnWadSortedMultiKeysFormat  /* "offroad_ann_sorted_multi_keys_wad" */,

    // TODO: this one is totally invalid. It corresponds to both normal omni & to offroad-compressed omni =(
    OmniIndexFormat                     /* "omni_index" */, // TODO: omni_index -> omni

    OmniVideoFormat                     /* "omni_video_index" */, // TODO: is OmniVideoFormat really a separate format?
    ErfFormat                           /* "erf" */,
    HostErfFormat                       /* "host_erf" */,
    RegHostErfFormat                    /* "reg_host_erf" */,
    RegErfFormat                        /* "reg_erf" */,
    SentFormat                          /* "sent" */,
    AnnSentFormat                       /* "ann_sent" */,
    FactorAnnSentFormat                 /* "factor_ann_sent" */,
    LinkAnnSentFormat                   /* "link_ann_sent" */,
    FrqFormat                           /* "frq" */,
    MegaWadFormat                       /* "mega_wad" */,
    IndexAttrsFormat                    /* "index_attrs" */,
    DocAttrsFormat                      /* "doc_attrs" */,
    DocAttrs64Format                    /* "doc_attrs64" */,
    CategToNameFormat                   /* "categ_to_name" */,
    InvHashFormat                       /* "invhash" */,
    HashedKeyInvFormat                  /* "hashed_keyinv" */,

    HnswFormat                          /* "hnsw" */,

    VideoErfFormat                      /* "video_erf" */,
    VideoRegErfFormat                   /* "video_reg_erf" */,
    VideoHostRegErfFormat               /* "video_host_reg_erf" */,
    VideoAuthorRegErfFormat             /* "video_author_reg_erf" */,
    VideoTopRegErfFormat                /* "video_top_reg_erf" */,

    WebItdItpSlimIndexFormat            /* "web_itditp_slim_index" */,
};

} // namespace NDoom
