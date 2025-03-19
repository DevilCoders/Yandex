#pragma once

#include "spans.h"

#include <library/cpp/packedtypes/packedfloat.h>


#include <library/cpp/charset/wide.h>

#include <library/cpp/string_utils/base64/base64.h>

namespace NSegm {


enum ESegSpanBytes {
    SegSpanBytes_1 = 37,
    SegSpanBytes_2 = 25,
};

enum EStoreSegSpanVersion {
    SegSpanVersion_1 = 0,
    SegSpanVersion_2 = 1,
    CurrentStoredSegSpanVersion = SegSpanVersion_2,
};

const ui32 SegSpanBytesByVersion[] = { SegSpanBytes_1, SegSpanBytes_2 };

enum EBlockStepType {
    BST_INLINE = 0, BST_ITEM, BST_SUPITEM, BST_PARAGRAPH, BST_BLOCK
};

struct TBlockInfo {
    enum {
        MaxBlocks = 3, MaxParagraphs = 3, MaxItems = 3, MaxDepth = 0x7f
    };

    union {
        struct {
            ui8 Depth :7;
        };

        struct {
            ui8 HasSuperItem :1;
            ui8 Items :2;
            ui8 Paragraphs :2;
            ui8 Blocks :2;
            ui8 Included :1;
        };
    };
public:
    static TBlockInfo Make(bool included = 0);

    static TBlockInfo Make(ui32 blocks, ui32 pars, ui32 items, bool hassuper = 0, bool included = 0);

    friend bool operator==(const TBlockInfo& a, const TBlockInfo& b) {
        return a.Depth == b.Depth;
    }

    friend bool operator!=(const TBlockInfo& a, const TBlockInfo& b) {
        return !(a == b);
    }

    ui8 Level() {
        return Items + Blocks + Paragraphs;
    }

    void Add(EBlockStepType w);

    TString ToString() const;
};

struct TBlockDist {
    union {
        ui32 Depth;
        struct {
            ui8 SuperItems;
            ui8 Items;
            ui8 Paragraphs;
            ui8 Blocks;
        };
    };

    TBlockDist()
        : Depth()
    {
    }

    TBlockDist(TBlockInfo a, TBlockInfo b);

    ui32 StructDist() const {
        return Paragraphs + Blocks;
    }

    static TBlockDist Max();

    static TBlockDist Make(ui32 blocks = 0, ui32 pars = 0, ui32 items = 0, ui32 super = 0);
};

#pragma pack(push, 1)
struct TStoreSegmentSpanData {
    static const ui32 NBytes = SegSpanBytes_2;
    union {
        char Bytes[NBytes];

        struct {
            TBlockInfo FirstBlock;//1
            TBlockInfo LastBlock;//1
            ui16 Words;//2
            ui16 LinkWords;//2
            ui8 Links;//1
            ui8 LocalLinks;//1
            ui8 Domains;//1
            ui8 Inputs; //1
            ui8 Blocks; //1

            union {
                ui8 AllMarkers;
                struct {
                    ui8 AdsCSS :1;
                    ui8 AdsHeader :1;
                    ui8 FooterCSS :1;
                    ui8 CommentsCSS :1;
                    ui8 PollCSS :1;
                    ui8 MenuCSS :1;
                    ui8 IsHeader :1;
                    ui8 HasHeader :1;
                };
            }; //1
            union {
                ui8 AllMarkers3;
                struct {
                    ui8 MainContentFrontAdjoining :1;
                    ui8 MainContentBackAdjoining :1;
                    ui8 MainContentFront :1;
                    ui8 MainContentBack :1;
                    ui8 InMainContentNews :1;
                    ui8 HasMainHeaderNews :1;
                };
            }; // 1
            ui8 HeadersCount; //1
            ui8 TitleWords; //1 = 15
            ui16 CommasInText; //2
            ui16 SpacesInText; //2
            ui16 AlphasInText; //2
            ui16 SymbolsInText; //2
            ui16 MainWeight; //2 = 25
        };
    };

    TStoreSegmentSpanData() {
        Zero(Bytes);
    }
};

struct TSegmentSpan: TSpan {
    static const ui32 NBytes = SegSpanBytes_1;

    union {
        char Bytes[NBytes];

        struct {
            //========== Version 1 begins here =============
            //signature of the segment's block path.
            //like hash(front block dom path) << 16 | hash(back block dom path)
            union {
                ui32 Signature;
                struct {
                    ui16 FirstSignature;
                    ui16 LastSignature;
                };
            };//4

            //if front block is included + distance to the nearest common parent
            //with prev. segment's back block
            TBlockInfo FirstBlock;//1
            //if back block is included + distance to the nearest common parent
            //with next segment's front block
            TBlockInfo LastBlock;//1

            ui16 Words;//2
            ui16 LinkWords;//2
            ui8 Links;//1
            ui8 LocalLinks;//1
            ui8 Domains;//1
            ui8 Inputs;//1
            ui8 Blocks;//1

            union {
                ui8 AllMarkers;
                struct {
                    ui8 AdsCSS :1;
                    ui8 AdsHeader :1;
                    ui8 FooterCSS :1;
                    ui8 FooterText :1;
                    ui8 PollCSS :1;
                    ui8 MenuCSS :1;
                    ui8 IsHeader :1;
                    ui8 HasHeader :1;
                };
            };//1

            //========== Version 2 begins here =============
            union {
                ui8 AllMarkers2;
                struct {
                    ui8 CommentsCSS :1;
                    ui8 CommentsHeader :1;
                    ui8 AdsHref :1;
                    ui8 InArticle :1;
                    ui8 InReadabilitySpans :1;
                    ui8 InMainContentNews :1;
                    ui8 HasMainHeaderNews :1;
                    ui8 HasSelfLink :1;
                };
            };//1 = 17

            //========== Version 4 begins here =============
            ui8 FirstTag;//1 = 18
            ui8 LastTag; //1 = 19
            ui8 FrontAbsDepth; //1 = 20
            ui8 BackAbsDepth; //1 = 21
            ui8 HeadersCount; //1 = 22

            ui8 TitleWords; //1 = 23
            ui8 FooterWords; //1 = 24
            ui8 WordsInHeaders; //1 = 25
            ui8 FragmentLinks; // 1 = 26

            ui16 CommasInText;//2 = 28
            ui16 SpacesInText;//2 = 30
            ui16 AlphasInText;//2 = 32
            ui16 SymbolsInText;//2 = 34

            //========== Version 5 begins here =============
            union {
                ui8 AllMarkers3;
                struct {
                    ui8 MainContentFrontAdjoining :1;
                    ui8 MainContentBackAdjoining :1;
                    ui8 MainContentFront :1;
                    ui8 MainContentBack :1;
                };
            };//1 = 35

            ui16 MainWeight;//2 = 37
            ui8 MetaDescrWords; //1 = 38
        };
    };

    ui8 Type;
    ui16 Number;
    ui16 Total;
    float Weight;

public:
    explicit TSegmentSpan(TPosting begin = 0, TPosting end = 0);

    ui32 GetMarkers() const {
        return AllMarkers | (AllMarkers2 << 8);
    }

    float GetMainContentWeight() const {
        return f16(MainWeight);
    }

    void SetMarkers(ui32 markers) {
        AllMarkers = markers & 0xFF;
        AllMarkers2 = (markers >> 8) & 0xFF;
    }

    void MergePrev(const TSegmentSpan& prev);
    void MergeNext(const TSegmentSpan& next);

    float AvLocalLinksPerLink() const {
        return Av(LocalLinks, Links, 1, 1);
    }

    float AvWordsPerDomain() const {
        return Av(Words, Domains);
    }

    float AvWordsPerLink() const {
        return Av(Words, Links);
    }

    float AvWordsPerInput() const {
        return Av(Words, Inputs);
    }

    float AvLinkWordsPerWord() const {
        return Av(LinkWords, Words, 0, 0);
    }

    float AvWordsPerBlock() const {
        return Av(Words, Blocks, 0, 0);
    }

    TString ToString(bool verbose = false) const;

    bool MainContentCandidate() const {
        return Words && SymbolsInText && Sentences();
    }

public:
    template<typename T>
    static void CheckedAdd(T& a, ui32 b) {
        a = (T)Min<ui32>(Max<T>(), a + b);
    }

private:

    void MergeWith(const TSegmentSpan& seg);

    static float Av(ui32 a, ui32 b, float b0 = Max<float> (), float a0b0 = Max<float> ()) {
        return b ? float(a) / b : a ? b0 : a0b0;
    }
};
#pragma pack(pop)

TStoreSegmentSpanData CreateStoreSegmentSpanData(const TSegmentSpan& sp);
void FillSegmentSpanFromStoreSegmentSpanData(const TStoreSegmentSpanData& data, TSegmentSpan& sp);

inline bool DecodeSegmentSpan(TSegmentSpan& sp, const char* bytes, size_t len, ui8 segVersion) {
    Y_ASSERT(Y_ARRAY_SIZE(SegSpanBytesByVersion) > segVersion);
    size_t nbytes = Min<size_t>(SegSpanBytesByVersion[segVersion], len);

    if (!bytes) {
        Clog << __FILE__ << ":" << __LINE__;
        if (!bytes) {
            Clog << " null bytes";
        }

        if (!EqualToOneOf(nbytes, SegSpanBytes_1, SegSpanBytes_2)) {
            Clog << " invalid data length (" << len << "), must be one of these: "
            << (int)SegSpanBytes_1 << ", "
            << (int)SegSpanBytes_2;
        }

        Clog << Endl;
        return false;
    }

    if (segVersion == CurrentStoredSegSpanVersion) {
        TStoreSegmentSpanData data;
        memcpy(data.Bytes, bytes, nbytes);
        FillSegmentSpanFromStoreSegmentSpanData(data, sp);
    } else {
        memcpy(sp.Bytes, bytes, nbytes);
    }
    sp.Weight = sp.GetMainContentWeight();
    return true;
}

inline void DecodeSegmentSpan(TSegmentSpan& sp, TStringBuf s, ui8 segVersion) {
    DecodeSegmentSpan(sp, s.data(), s.size(), segVersion);
}

inline TString DecodeOldSegmentAttributes(const wchar16* attr, ui8 segVersion) {
    const size_t bsz2 = SegSpanBytesByVersion[segVersion];
    const size_t bsz1 = Base64EncodeBufSize(bsz2) - 1;
    TTempBuf tmp1(bsz1 + 1), tmp2(bsz2);

    WideToChar(attr, bsz1, tmp1.Data(), CODES_YANDEX);
    return TString(Base64Decode(TStringBuf(tmp1.Data(), bsz1), tmp2.Data()));
}

inline bool DecodeSegmentSpanFromAttr(TSegmentSpan& sp, const wchar16* attr, ui8 segVersion) {
    TString raw = DecodeOldSegmentAttributes(attr, segVersion);
    return DecodeSegmentSpan(sp, raw.data(), raw.size(), segVersion);
}

}
