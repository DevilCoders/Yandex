#include "qd_cgi_strings.h"
#include "qd_cgi_utils.h"

#include <kernel/hosts/owner/owner.h>
#include <kernel/querydata/idl/scheme/querydata_request.sc.h>
#include <kernel/urlid/url2docid.h>

#include <library/cpp/json/json_prettifier.h>
#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/streams/lz/lz.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/relaxed_escaper/relaxed_escaper.h>

#include <util/generic/ymath.h>
#include <util/generic/algorithm.h>
#include <util/stream/buffer.h>
#include <util/stream/zlib.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/builder.h>
#include <util/string/cast.h>

namespace NQueryData {

    ui32 CalculatePartsCount(const TRequestSplitMeasure& req, const TRequestSplitLimits& limits) {
        ui32 maxreqparts = Max(1u, limits.MaxParts);
        ui32 maxreqitems = Max(1u, limits.MaxItems);
        ui32 maxreqlength = Max(1u, limits.MaxLength);

        ui32 nparts = 0;
        ui32 szquota = Max(maxreqlength, req.SharedSize) - req.SharedSize;

        if (!req.SplittableSize && !req.SplittableCount) {
            // nothing to split
            nparts = 1;
        } else if (!szquota || szquota * maxreqparts < req.SplittableSize) {
            // too many items or no space left, max split
            nparts = maxreqparts;
        } else {
            // split to balance size of requests
            nparts = Max<ui32>(ceil(float(req.SplittableSize) / szquota), 1);
        }

        // trying to satisfy item count constraint
        if (req.SplittableCount) {
            nparts = Max<ui32>(nparts, ceil(float(req.SplittableCount) / maxreqitems));
        }

        // trying to satisfy parts count constraint
        nparts = Min<ui32>(nparts, maxreqparts);

        // adding some sanity
        if (req.SplittableCount) {
            nparts = Min<ui32>(nparts, req.SplittableCount);
        }

        return nparts;
    }

    template <typename T>
    ui32 DoCountSize(const TVector<T>& items) {
        ui32 res = 0;
        for (const auto& item : items) {
            res += item.size() + 3;
        }
        return res;
    }

    // for urls we expect that base64 and compression cancel each other
    ui32 CountSize(const TStringBufs& items) {
        return DoCountSize(items);
    }

    ui32 CountSize(const TVector<TString>& items) {
        return DoCountSize(items);
    }

    std::pair<ui32, ui32> GetIndexRange(ui32 size, ui32 part, ui32 parts) {
        ui32 quotient = (ui32)(size / parts);
        ui32 remnant = (ui32)(size % parts);
        ui32 remnantOffset = parts - remnant;
        ui32 offset0 = part * quotient + (part >= remnantOffset ? part - remnantOffset : 0);
        ui32 offset1 = offset0 + quotient + (part >= remnantOffset ? 1 : 0);
        return std::make_pair(offset0, offset1);
    };

    template <typename T>
    static TString DoVectorToStringSplit(const TVector<T>& vec, char sep, ui32 part, ui32 parts) {
        if (vec.empty() || !parts || part >= parts) {
            return TString();
        }

        if (vec.size() <= parts) {
            if (part < vec.size()) {
                return ToString(vec[part]);
            } else {
                return TString();
            }
        }

        const auto range = GetIndexRange((ui32)vec.size(), part, parts);

        TString result;

        if (range.first == range.second) {
            return result;
        }

        result = vec[range.first];

        for (ui32 i = range.first + 1; i < range.second; ++i) {
            result.append(sep).append(vec[i]);
        }

        return result;
    }

    TString VectorToString(const TStringBufs& vec, char sep) {
        return DoVectorToStringSplit(vec, sep, 0, 1);
    }

    TString VectorToString(const TVector<TString>& vec, char sep) {
        return DoVectorToStringSplit(vec, sep, 0, 1);
    }

    TString VectorToStringSplit(const TStringBufs& vec, char sep, ui32 part, ui32 parts) {
        return DoVectorToStringSplit(vec, sep, part, parts);
    }

    TString VectorToStringSplit(const TVector<TString>& vec, char sep, ui32 part, ui32 parts) {
        return DoVectorToStringSplit(vec, sep, part, parts);
    }


    void SetCgi(TString& res, TStringBuf name, TString buf, char sep) {
        Quote(buf);

        if (!!buf) {
            res.append(name).append('=').append(buf).append(sep);
        }
    }

    void SetCgiRoot(TString& res, TStringBuf name, const TString& val) {
        SetCgi(res, name, val, '&');
    }

    void SetCgiRoot(TString& res, TStringBuf name, TStringBuf val) {
        SetCgiRoot(res, name, TString{val});
    }

    void SetCgiSub(TString& res, TStringBuf name, const TString& val) {
        SetCgi(res, name, val, ';');
    }

    void SetCgiSub(TString& res, TStringBuf name, TStringBuf val) {
        SetCgiSub(res, name, TString{val});
    }

    int ParseCompressedCgiPropName(TStringBuf& name, ECgiCompression& comprType, TStringBuf cgiName) {
        name = cgiName.NextTok(':');
        TStringBuf ver = cgiName.RNextTok(':');
        TStringBuf comprTypeStr = cgiName.NextTok(':');
        comprType = comprTypeStr ? FromStringWithDefault(comprTypeStr, CC_INVALID) : CC_PLAIN;
        int n = 0;
        if (!TryFromString(ver, n)) {
            return 0;
        } else {
            return n;
        }
    }

    TString FormCompressedCgiPropName(const TStringBuf name, const ECgiCompression comprType, const int ver) {
        TStringBuilder pName;
        pName << name << ":";
        if (comprType > CC_PLAIN) {
            pName << comprType << ":";
        }
        pName << ver;
        return pName;
    }

    template <class TCompr>
    TString DoCompress(TStringBuf in) {
        TStringStream ss;
        {
            TCompr compr(&ss);
            compr.Write(in);
        }
        return ss.Str();
    }

    template <class TDecompr>
    TString DoDecompress(TStringBuf in) {
        TStringStream ss;
        {
            TMemoryInput minp(in.data(), in.size());
            TDecompr decompr(&minp);
            TransferData(&decompr, &ss);
        }
        return ss.Str();
    }

    TString CompressCgi(const TStringBuf in, const ECgiCompression comprType) {
        TString result;
        switch (comprType) {
        case CC_PLAIN:
            result = in;
            Quote(result, "/:,{}\"");
            return result;
        case CC_ZLIB:
            result = DoCompress<TZLibCompress>(in);
            break;
        case CC_LZ4:
            result = DoCompress<TLz4Compress>(in);
            break;
        case CC_BASE64:
            result = in;
            break;
        default:
            Y_ENSURE(false, "invalid compression " << comprType);
        }
        return Base64EncodeUrl(result);
    }

    TString CompressCgi(const TStringBuf in, const TStringBuf comprType) {
        return CompressCgi(in, FromStringWithDefault(comprType, CC_INVALID));
    }

    bool DecompressCgi(TString& out, const TStringBuf in, const ECgiCompression comprType) {
        try {
            out = in;
            if (comprType > CC_PLAIN) {
                out.resize(AlignUp<size_t>(out.size(), 4), '=');
                out = Base64Decode(out);
            }
            switch (comprType) {
            case CC_PLAIN:
                CGIUnescape(out);
                return true;
            case CC_BASE64:
                return true;
            case CC_ZLIB:
                out = DoDecompress<TZLibDecompress>(out);
                return true;
            case CC_LZ4:
                out = DoDecompress<TLz4Decompress>(out);
                return true;
            default:
                return false;
            }
        } catch (const yexception&) {
            return false;
        }
    }

    bool DecompressCgi(TString& out, const TStringBuf in, const TStringBuf comprType) {
        return DecompressCgi(out, in, FromStringWithDefault(comprType, CC_INVALID));
    }

    TString DecompressCgi(const TStringBuf in, const ECgiCompression comprType) {
        TString out;
        if (DecompressCgi(out, in, comprType)) {
            return out;
        } else {
            return TString();
        }
    }

    TString DecompressCgi(const TStringBuf in, const TStringBuf comprType) {
        return DecompressCgi(in, FromStringWithDefault(comprType, CC_INVALID));
    }

    void InsertToSet(TSet<TString>& tgt, const TStringBufs& src) {
        for (const auto& c : src) {
            tgt.insert(TString{c});
        }
    }

    bool NextToken(TStringBuf& tok, TStringBuf& lst, char sep) {
        if (!lst) {
            return false;
        }

        do {
            tok = lst.NextTok(sep);
        } while (!tok && lst);

        return !!tok;
    }

    TString JsonToString(const NSc::TValue& val) {
        return NJson::CompactifyJson(val.ToJson(true), true);
    }

}
