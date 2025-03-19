#pragma once

#include <util/generic/bitmap.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets {

extern const TString SNIP_MARKERS_NAME;

// Order matters, please, do not sort
enum EMarker {
    MRK_YOUTUBE_CHANNEL = 0         /* "youtube_channel" */,
    MRK_TABLESNIP                   /* "table_snip" */,
    MRK_TABLESNIP_CTR               /* "table_snip_ctr" */,
    MRK_CREATIVE_WORK               /* "creative_work" */,
    MRK_ENCYC                       /* "encyc" */,
    MRK_KINOPOISK                   /* "kinopoisk" */,
    MRK_LITERA                      /* "litera" */,
    MRK_MEDIAWIKI                   /* "mediawiki" */,
    MRK_PRODUCT_OFFER               /* "product_offer" */,
    MRK_QUESTION                    /* "question" */,
    MRK_SCHEMA_MOVIE                /* "schema_movie" */,
    MRK_STATIC_ANNOTATION           /* "static_annotation" */,
    MRK_TRASH_ANNOTATION            /* "trash_annotation" */,
    MRK_WEIGHTED_METADESCR_AND_DMOZ /* "metadescr_and_dmoz" */,
    MRK_WEIGHTED_YACA               /* "weighted_yaca" */,
    MRK_NEWS                        /* "news" */,
    MRK_FORUM                       /* "forum" */,
    MRK_LIST                        /* "list" */,
    MRK_SCHEMA_VTHUMB               /* "schema_vthumb" */,

    MRK_FOTO_RECIPE                 /* "foto_recipe" */,
    MRK_DIC_ACADEMIC                /* "dic_academic" */,
    MRK_DMOZ                        /* "dmoz" */,
    MRK_EKSISOZLUK_SNIP             /* "eksisozluk_snip" */,
    MRK_EXTENDED                    /* "extended" */,
    MRK_FAKE_REDIRECT               /* "fake_redirect" */,
    MRK_MOBILE_EXTRA                /* "mobile_extra" */,
    MRK_HILITED_URL                 /* "hilited_url" */,
    MRK_KINO                        /* "kino" */,
    MRK_LASTFM                      /* "lastfm" */,
    MRK_MARKET                      /* "market" */,
    MRK_MOIKRUG                     /* "moikrug" */,
    MRK_MYSPACE                     /* "myspace" */,
    MRK_OGTEXT                      /* ogtext */,
    MRK_PRESSPORT                   /* "pressport" */,
    MRK_PREVIEW                     /* "preview" */,
    MRK_RABOTA                      /* "rabota" */,
    MRK_RECIPE                      /* "recipe" */,
    MRK_REFERAT                     /* "referat" */,
    MRK_REVIEW                      /* "review" */,
    MRK_ROBOTS_TXT                  /* "robots_txt" */,
    MRK_SAHIBINDEN                  /* "sahibinden" */,
    MRK_SAHIBINDEN_FAKE             /* "sahibinden_fake" */,
    MRK_META_DESCR                  /* "meta_descr" */,
    MRK_SITELINKS_BASE              /* "sitelinks_base" */,
    MRK_SOFTWARE                    /* "software" */,
    MRK_SUPPRESS_CYRILLIC           /* "suppress_cyrillic" */,
    MRK_TEXT_FOR_NPS                /* "text_for_nps" */,
    MRK_TITLE_TO_SNIP               /* "title_to_snip" */,
    MRK_TORRENT_FILM                /* "torrent_film" */,
    MRK_TWITTER                     /* "twitter" */,
    MRK_URL_MENU                    /* "url_menu" */,
    MRK_VIDEO                       /* "video" */,
    MRK_YACA                        /* "yaca" */,
    MRK_YOUTUBE                     /* "youtube" */,
    MRK_YOUTUBE_CHANNEL_IMG         /* "youtube_channel_img" */,
    MRK_ISNIP                       /* "isnip" */,
    MRK_VIDEO_DESCR                 /* "videodescr" */,
    MRK_REMOVE_EMOJI                /* "remove_emoji" */,
    MRK_COUNT
};

float GetMarkerWeight(EMarker marker);

class TMarkersMask: public TDynBitMap {
public:
    void SetMarker(const EMarker mark, bool value = true) {
        if (value)
            Set(mark);
        else
            Reset(mark);
    }
    bool HasMarker(const EMarker mark) const {
        return Get(mark);
    }
    TString Dump() const;
    bool Parse(const TStringBuf& text);
    bool Load(const TStringBuf& b64Data);
};

TString DebugString(const TMarkersMask& markers);
TString PrintEnabled(const TMarkersMask& markers);
bool ReplaceMarkers(TString& text, const TMarkersMask& markers);

double WinProbability(const TVector<double>& winProbs);
double MarkersKPI(const TMarkersMask& markers);

}; // end of namespace

const TString& ToString(NSnippets::EMarker value);
