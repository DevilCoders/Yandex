#include "tarcface.h"

#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/packers/packers.h>

#include <kernel/segmentator/structs/segment_span_decl.h>

#include <kernel/tarc/protos/archive_zone_attributes.pb.h>

#include <util/generic/utility.h>
#include <util/generic/yexception.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/stream/buffer.h>
#include <util/ysaveload.h>

#include <google/protobuf/descriptor.h>

static_assert(sizeof(TArchiveTextHeader) == 8, "sizeof(TArchiveTextHeader) == 8");
static_assert(sizeof(TArchiveTextBlockInfo) == 8, "sizeof(TArchiveTextBlockInfo) == 8");

enum EArcZoneAttrVersion {
    ArcZoneAttrVersion_1 = 0,
    ArcZoneAttrVersion_2 = 1,
    ArcZoneAttrVersion_3 = 2,
    CurrentArcZoneAttrVersion = ArcZoneAttrVersion_2
};

enum EArcSentInfosVersion {
    ArcSentInfosVersion_1 = 0,
    ArcSentInfosVersion_2 = 1,
    CurrentArcSentInfosVersion = ArcSentInfosVersion_2,
};

static void DeltaEncodeArchiveZones(
        const TVector<TArchiveZoneSpan>& spans,
        TVector<TArchiveZoneSpan>& out)
{
    out = spans;
    const size_t size = spans.size();
    if (size <= 1) {
        return;
    }
    for (size_t i = 1; i < size; ++i) {
        out[i].SentEnd -= out[i].SentBeg;
        out[i].SentBeg -= spans[i - 1].SentBeg;
    }
}

static void DeltaEncodeSentInfos(
        TConstArrayRef<TArchiveTextBlockSentInfo> infos,
        TVector<TArchiveTextBlockSentInfo>& out)
{
    out = TVector<TArchiveTextBlockSentInfo>(infos.begin(), infos.end());
    const size_t size = infos.size();
    if (size <= 1) {
        return;
    }
    for (size_t i = 1; i < size; ++i) {
        out[i].OrigOffset -= infos[i - 1].OrigOffset;
        out[i].EndOffset -= infos[i - 1].EndOffset;
    }
}

static void DeltaDecodeArchiveZones(
        TVector<TArchiveZoneSpan>& spans)
{
    const size_t size = spans.size();
    for (size_t i = 1; i < size; ++i) {
        spans[i].SentBeg += spans[i - 1].SentBeg;
        spans[i].SentEnd += spans[i].SentBeg;
    }
}

static void DeltaDecodeSentInfos(
        TArchiveTextBlockSentInfo* sis,
        ui32 siCount)
{
    for (ui32 i = 1; i < siCount; ++i) {
        sis[i].OrigOffset += sis[i - 1].OrigOffset;
        sis[i].EndOffset += sis[i - 1].EndOffset;
    }
}

void SaveArchiveTextBlockSentInfos(
        TConstArrayRef<TArchiveTextBlockSentInfo> infos,
        IOutputStream* out)
{
    TVector<TArchiveTextBlockSentInfo> deltaInfos;
    DeltaEncodeSentInfos(infos, deltaInfos);
    TVector<char> deltaBuffer;
    NArc::GetSentInfosCompressor().Compress(
                TStringBuf((const char*)deltaInfos.data(),
                           deltaInfos.size() * sizeof(TArchiveTextBlockSentInfo)),
                &deltaBuffer);
    if (CurrentArcSentInfosVersion == ArcSentInfosVersion_2 &&
        deltaBuffer.size() + 4 < infos.size() * sizeof(TArchiveTextBlockSentInfo))
    {
        ui32 len = deltaBuffer.size() + sizeof(ui32);
        ::Save(out, len);
        static_assert(sizeof(NArc::COMPR_MAGIC_NUMBER_V1) == sizeof(ui32), "Wrong magic number size");
        len = NArc::COMPR_MAGIC_NUMBER_V1;
        ::Save(out, len);
        out->Write(deltaBuffer.data(), deltaBuffer.size());
    } else {
        ui32 len = infos.size() * sizeof(TArchiveTextBlockSentInfo);
        ::Save(out, len);
        out->Write(infos.data(), len);
    }
}

void LoadArchiveTextBlockSentInfos(
        TMemoryInput* in,
        TStringBuf* infos,
        TVector<char>* tempBuffer)
{
    ui32 len = 0;
    ::Load(in, len);
    if (len == 0) {
        infos->Clear();
        return;
    }
    {
        const char* data = in->Buf();
        const size_t realLen = in->Skip(len);
        Y_ASSERT(realLen == len);
        *infos = TStringBuf(data, len);
    }
    static_assert(sizeof(NArc::COMPR_MAGIC_NUMBER_V1) == sizeof(ui32), "Wrong magic number size");
    if (infos->size() >= sizeof(ui32) &&
        *((const ui32*)infos->data()) == NArc::COMPR_MAGIC_NUMBER_V1)
    {
        *infos = TStringBuf(infos->data() + sizeof(ui32), len - sizeof(ui32));
        tempBuffer->clear();
        NArc::GetSentInfosDecompressor().Decompress(
                    *infos, tempBuffer);
        Y_ASSERT(tempBuffer->size() % sizeof(TArchiveTextBlockSentInfo) == 0);
        DeltaDecodeSentInfos(
                    (TArchiveTextBlockSentInfo*)tempBuffer->data(),
                    tempBuffer->size() / sizeof(TArchiveTextBlockSentInfo));
        *infos = TStringBuf(tempBuffer->data(), tempBuffer->size());
    }
}

void TArchiveWeightZones::SaveAsIs(TBufferOutput* out) const {
    LowZone.SaveAsIs(out, false);
    HighZone.SaveAsIs(out, false);
    BestZone.SaveAsIs(out, false);
}

void TArchiveWeightZones::SaveDelta(TVector<char>* out, ui32 expectedSize) const {
    TBufferOutput tempOutput(expectedSize);
    LowZone.SaveDelta(&tempOutput, false);
    HighZone.SaveDelta(&tempOutput, false);
    BestZone.SaveDelta(&tempOutput, false);
    const TBuffer rawBuffer = tempOutput.Buffer();
    NArc::GetMarkupZonesCompressor().Compress(
                TStringBuf(rawBuffer.Data(),
                           rawBuffer.Size()),
                out);
}

void TArchiveWeightZones::Save(IOutputStream* rh) const {
    TBufferOutput asisOutput;
    SaveAsIs(&asisOutput);
    const TBuffer& asisBuffer = asisOutput.Buffer();

    TVector<char> deltaBuffer;
    SaveDelta(&deltaBuffer, asisBuffer.Size());

    if (CurrentArcZoneAttrVersion == ArcZoneAttrVersion_3 &&
        deltaBuffer.size() + 4 < asisBuffer.Size())
    {
        ui32 len = deltaBuffer.size() + sizeof(ui32);
        ::Save(rh, len);
        static_assert(sizeof(NArc::COMPR_MAGIC_NUMBER_V1) == sizeof(ui32), "Wrong magic number size");
        len = NArc::COMPR_MAGIC_NUMBER_V1;
        ::Save(rh, len);
        rh->Write(deltaBuffer.data(), deltaBuffer.size());
    } else {
        ui32 len = asisBuffer.Size();
        ::Save(rh, len);
        rh->Write(asisBuffer.Data(), asisBuffer.Size());
    }
}

static void LoadOneWeightZone(
        IInputStream* in,
        TVector<TArchiveZoneSpan>& spans,
        EArcZoneAttrVersion version)
{
    TArchiveZone temp;
    if (version == ArcZoneAttrVersion_3) {
        temp.LoadDelta(in, false);
    } else {
        temp.LoadAsIs(in, false);
    }
    spans.insert(spans.end(),
                 temp.Spans.begin(),
                 temp.Spans.end());
}

void TArchiveWeightZones::LoadAsIs(IInputStream* in) {
    LoadOneWeightZone(in, LowZone.Spans, ArcZoneAttrVersion_2);
    LoadOneWeightZone(in, HighZone.Spans, ArcZoneAttrVersion_2);
    LoadOneWeightZone(in, BestZone.Spans, ArcZoneAttrVersion_2);
}

void TArchiveWeightZones::LoadDelta(TMemoryInput* in, ui32 len) {
    const char* compressedPtr = in->Buf();
    const size_t compressedLen = in->Skip(len);
    Y_ASSERT(compressedLen == len);

    TStringBuf compressed(compressedPtr, compressedLen);
    TVector<char> decompressed;
    decompressed.reserve(compressedLen * 2);
    NArc::GetMarkupZonesDecompressor().Decompress(
                compressed,
                &decompressed);

    TMemoryInput decompressedIn(decompressed.data(), decompressed.size());

    LoadOneWeightZone(in, LowZone.Spans, ArcZoneAttrVersion_3);
    LoadOneWeightZone(in, HighZone.Spans, ArcZoneAttrVersion_3);
    LoadOneWeightZone(in, BestZone.Spans, ArcZoneAttrVersion_3);

    Y_ASSERT(decompressedIn.Exhausted());
}

void TArchiveWeightZones::Load(TMemoryInput* in) {
    ui32 len = 0;
    ::Load(in, len);
    if (len == 0) {
        return;
    }
    static_assert(sizeof(NArc::COMPR_MAGIC_NUMBER_V1) == sizeof(ui32), "Wrong magic number size");
    if (in->Avail() >= sizeof(NArc::COMPR_MAGIC_NUMBER_V1) &&
        *((const ui32*)in->Buf()) == NArc::COMPR_MAGIC_NUMBER_V1)
    {
        in->Skip(sizeof(NArc::COMPR_MAGIC_NUMBER_V1));
        len -= sizeof(NArc::COMPR_MAGIC_NUMBER_V1);

        LoadDelta(in, len);
    } else {
        LoadAsIs(in);
    }
}

const char ARC_ZONE_ATTR_VERSION_LABEL[] = "YZAL";

namespace NArchiveZoneAttr {
    namespace NAnchor {
        const char* LINK = "link";
    }
    namespace NAnchorInt {
        const char* LINKINT = "linkint";
        const char* SEGMLINKINT = "content_linkint";
    }
    namespace NTelephone {
        const char* COUNTRY_CODE = "tel_code_country";
        const char* AREA_CODE = "tel_code_area";
        const char* LOCAL_NUMBER = "tel_local";
    }
    namespace NSegm {
        const char* STATISTICS = "SegSt";
    }
    namespace NForum {
        const char* DATE = "forum_date";
        const char* AUTHOR = "forum_author";
        const char* ANCHOR = "forum_anchor";
        const char* QUOTE_AUTHOR = "quote_author";
        const char* QUOTE_URL = "quote_url";
        const char* QUOTE_DATE = "quote_date";
        const char* QUOTE_TIME = "quote_time";
        const char* NAME = "forum_name";
        const char* LINK = "forum_link";
        const char* DESCRIPTION = "forum_descr";
        const char* TOPICS = "forum_topics";
        const char* MESSAGES = "forum_messages";
        const char* VIEWS = "forum_views";
        const char* LAST_TITLE = "forum_last";
        const char* LAST_AUTHOR = "forum_last_author";
        const char* LAST_DATE = "forum_last_date";
    }
    namespace NXPath {
        const char* XPATH_ZONE_TYPE = "xpz_type";
    }
    namespace NDater {
        const char* DATE = "date_value";
    }
    namespace NUserReview {
        const char* AUTHOR     = "review_author";
        const char* DATE       = "review_date";
        const char* NORM_DATE  = "review_norm_date";
        const char* RATE       = "review_rate";
        const char* TITLE      = "review_title";
        const char* USEFULNESS = "review_usefulness";
    }
    namespace NFio {
        const char* FIO_LEN = "fio_len";
    }
    namespace NNumber {
        const char* MEASURENUM = "mnum";
    }
}

bool IsAttrAllowed(EArchiveZone zone, const char* attrName)
{
    bool res(false);
    switch(zone)
    {
        using namespace NArchiveZoneAttr;
        case AZ_TELEPHONE:
            res = stricmp(attrName, NTelephone::AREA_CODE) == 0;
            res |= stricmp(attrName, NTelephone::COUNTRY_CODE) == 0;
            res |= stricmp(attrName, NTelephone::LOCAL_NUMBER) == 0;
            break;
        case AZ_SEGAUX:
        case AZ_SEGCONTENT:
        case AZ_SEGCOPYRIGHT:
        case AZ_SEGHEAD:
        case AZ_SEGLINKS:
        case AZ_SEGMENU:
        case AZ_SEGREFERAT:
            res = stricmp(attrName, NArchiveZoneAttr::NSegm::STATISTICS) == 0;
            break;
        case AZ_FORUM_MESSAGE:
            res = stricmp(attrName, NForum::DATE) == 0;
            res |= stricmp(attrName, NForum::AUTHOR) == 0;
            res |= stricmp(attrName, NForum::ANCHOR) == 0;
            break;
        case AZ_XPATH:
            res = stricmp(attrName, NXPath::XPATH_ZONE_TYPE) == 0;
            break;
        case AZ_DATER_DATE:
            res = stricmp(attrName, NDater::DATE) == 0;
            break;
        case AZ_USER_REVIEW:
            res = stricmp(attrName, NUserReview::AUTHOR) == 0;
            res |= stricmp(attrName, NUserReview::DATE) == 0;
            res |= stricmp(attrName, NUserReview::NORM_DATE) == 0;
            res |= stricmp(attrName, NUserReview::RATE) == 0;
            res |= stricmp(attrName, NUserReview::TITLE) == 0;
            res |= stricmp(attrName, NUserReview::USEFULNESS) == 0;
            break;
        case AZ_FORUM_QBODY:
            res = stricmp(attrName, NForum::QUOTE_DATE) == 0;
            res |= stricmp(attrName, NForum::QUOTE_TIME) == 0;
            res |= stricmp(attrName, NForum::QUOTE_AUTHOR) == 0;
            res |= stricmp(attrName, NForum::QUOTE_URL) == 0;
            break;
        case AZ_ANCHORINT:
            res = stricmp(attrName, NAnchorInt::SEGMLINKINT) == 0;
            break;
        case AZ_FIO:
            res = stricmp(attrName, NFio::FIO_LEN) == 0;
            break;
        case AZ_FORUM_INFO:
            res = stricmp(attrName, NForum::NAME) == 0;
            res |= stricmp(attrName, NForum::LINK) == 0;
            res |= stricmp(attrName, NForum::DESCRIPTION) == 0;
            res |= stricmp(attrName, NForum::TOPICS) == 0;
            res |= stricmp(attrName, NForum::MESSAGES) == 0;
            res |= stricmp(attrName, NForum::LAST_TITLE) == 0;
            res |= stricmp(attrName, NForum::LAST_AUTHOR) == 0;
            res |= stricmp(attrName, NForum::LAST_DATE) == 0;
            break;
        case AZ_FORUM_TOPIC_INFO:
            res = stricmp(attrName, NForum::NAME) == 0;
            res |= stricmp(attrName, NForum::LINK) == 0;
            res |= stricmp(attrName, NForum::MESSAGES) == 0;
            res |= stricmp(attrName, NForum::VIEWS) == 0;
            res |= stricmp(attrName, NForum::LAST_TITLE) == 0;
            res |= stricmp(attrName, NForum::LAST_AUTHOR) == 0;
            res |= stricmp(attrName, NForum::LAST_DATE) == 0;
            res |= stricmp(attrName, NForum::AUTHOR) == 0;
            res |= stricmp(attrName, NForum::DATE) == 0;
            res |= stricmp(attrName, NForum::DESCRIPTION) == 0;
            break;
        case AZ_MEASURE:
            res = stricmp(attrName, NNumber::MEASURENUM) == 0;
            break;
        default:
            break;
    }
    return res;
}

void TArchiveZoneAttrs::Save(IOutputStream* out, TMultiMap<TUtf16String, ui64>& urls, ui8 segVersion) const
{
    using namespace google::protobuf;

    NProto::TArchiveZoneAttributes archiveZoneAttributes;
    archiveZoneAttributes.SetZoneId(ZoneId);
    for (TSpan2Attrs::const_iterator it = Span2AttrsWrapper.Span2Attrs.begin();
         it != Span2AttrsWrapper.Span2Attrs.end(); ++it)
    {
        const ui32 coord = it->first;
        NProto::TArchiveZoneAttributes::TZoneAttributes zoneAttr;
        const Descriptor* descriptor = zoneAttr.GetDescriptor();
        const Reflection* reflection = zoneAttr.GetReflection();
        for (THashMap<TString, TUtf16String>::const_iterator attrIt = it->second.begin();
             attrIt != it->second.end(); ++attrIt)
        {
            const char* attrName = attrIt->first.c_str();
            if (IsAttrAllowed((EArchiveZone)ZoneId, attrName))
            {
                if (ZoneId == AZ_ANCHORINT && strcmp((attrIt->first).data(), NArchiveZoneAttr::NAnchorInt::SEGMLINKINT) == 0) {
                    urls.insert(std::make_pair(attrIt->second, TZoneCoord(ZoneId, coord).ZoneCoord));
                } else if (strcmp((attrIt->first).data(), NArchiveZoneAttr::NSegm::STATISTICS) == 0) {
                    zoneAttr.SetSegSt(NSegm::DecodeOldSegmentAttributes((attrIt->second).data(), segVersion));
                } else {
                    const FieldDescriptor* text_field = descriptor->FindFieldByName(attrIt->first);
                    if (text_field != nullptr) {
                        reflection->SetString(&zoneAttr, text_field, WideToUTF8(attrIt->second));
                    }
                }
            }
        }
        TSpan2SegmentAttrs::const_iterator segAttrIt = Span2AttrsWrapper.Span2SegmentAttrs.find(coord);
        if (segAttrIt != Span2AttrsWrapper.Span2SegmentAttrs.end())
        {
            zoneAttr.SetSegSt(segAttrIt->second);
        }
        if (zoneAttr.ByteSize() > 0) {
            zoneAttr.SetCoord(coord);
            *archiveZoneAttributes.AddZoneAttributes() = zoneAttr;
        }
    }

    for (TSpan2SegmentAttrs::const_iterator segAttrIt = Span2AttrsWrapper.Span2SegmentAttrs.begin();
         segAttrIt != Span2AttrsWrapper.Span2SegmentAttrs.end(); ++segAttrIt)
    {
        const ui32 coord = segAttrIt->first;
        if (Span2AttrsWrapper.Span2Attrs.find(coord) != Span2AttrsWrapper.Span2Attrs.end()) {
            continue;
        }
        NProto::TArchiveZoneAttributes::TZoneAttributes zoneAttr;
        zoneAttr.SetSegSt(segAttrIt->second);
        zoneAttr.SetCoord(coord);
        *archiveZoneAttributes.AddZoneAttributes() = zoneAttr;
    }

    TBufferOutput res;
    archiveZoneAttributes.SerializeToArcadiaStream(&res);
    ::Save(out, res.Buffer());
}

void TArchiveZoneAttrs::LoadProtobuf(IInputStream* in)
{
    using namespace google::protobuf;

    TString attrStr;
    ::Load(in, attrStr);
    NProto::TArchiveZoneAttributes archiveZoneAttributes;
    bool succeeded = archiveZoneAttributes.ParseFromString(attrStr);
    Y_ASSERT(succeeded);
    if (!succeeded) {
        return;
    }
    ZoneId = archiveZoneAttributes.GetZoneId();
    for (size_t zoneIndex = 0; zoneIndex < archiveZoneAttributes.ZoneAttributesSize(); ++zoneIndex)
    {
        const NProto::TArchiveZoneAttributes::TZoneAttributes& zoneAttr = archiveZoneAttributes.GetZoneAttributes(zoneIndex);
        const Reflection* reflection = zoneAttr.GetReflection();
        TVector<const FieldDescriptor*> fieldDescriptors;
        reflection->ListFields(zoneAttr, &fieldDescriptors);
        for (size_t fieldIndex = 0; fieldIndex < fieldDescriptors.size(); ++fieldIndex)
        {
            TString value;
            if (fieldDescriptors[fieldIndex]->type() == FieldDescriptor::TYPE_BYTES ||
                fieldDescriptors[fieldIndex]->type() == FieldDescriptor::TYPE_STRING) {
                value = reflection->GetString(zoneAttr, fieldDescriptors[fieldIndex]);
            }
            const TString& attrName = fieldDescriptors[fieldIndex]->name();
            if (!value.empty()) {
                if (strcmp(attrName.data(), NArchiveZoneAttr::NSegm::STATISTICS) == 0) {
                    Span2AttrsWrapper.Span2SegmentAttrs[zoneAttr.GetCoord()] = value;
                } else {
                    Span2AttrsWrapper.Span2Attrs[zoneAttr.GetCoord()][attrName] = UTF8ToWide(value);
                }
            }
        }
    }
}

void TArchiveZoneAttrs::Load(IInputStream* in, ui8 arcZoneAttrVersion) {
    if (arcZoneAttrVersion == ArcZoneAttrVersion_1) {
        ::Load(in, ZoneId);
        ::Load(in, Span2AttrsWrapper.Span2Attrs);
    } else {
        LoadProtobuf(in);
    }
}

TSpanAttributes TArchiveZoneAttrs::GetSpanAttrs(const TArchiveZoneSpan& span) {
    return Span2AttrsWrapper.GetSpanAttrs(span.SentBeg, span.OffsetBeg);
}

TConstSpanAttributes TArchiveZoneAttrs::GetSpanAttrs(const TArchiveZoneSpan& span) const {
    return Span2AttrsWrapper.GetSpanAttrs(span.SentBeg, span.OffsetBeg);
}

TSpanAttributes TArchiveZoneAttrs::GetSpanAttrs(ui16 sent, ui16 off) {
    return Span2AttrsWrapper.GetSpanAttrs(sent, off);
}

TConstSpanAttributes TArchiveZoneAttrs::GetSpanAttrs(ui16 sent, ui16 off) const {
    return Span2AttrsWrapper.GetSpanAttrs(sent, off);
}

void TArchiveZoneAttrs::AddSpanAttr(ui16 sent, ui16 off, const char* name, const wchar16* val) {
    AddSpanAttr(TCoord(sent, off).Coord, name, val);
}

void TArchiveZoneAttrs::AddSpanAttr(ui32 setnAndOff, const TString& name, const TUtf16String& value) {
    Span2AttrsWrapper.Span2Attrs[setnAndOff][name] = value;
}

void TArchiveZoneAttrs::SwapAttrs(ui16 sent, ui16 off, TSpanAttributes& spanAttributes) {
    if (spanAttributes.AttrsHash != nullptr)
        Span2AttrsWrapper.Span2Attrs[TCoord(sent, off).Coord].swap(*spanAttributes.AttrsHash);

    if (spanAttributes.SegmentAttrs != nullptr)
        Span2AttrsWrapper.Span2SegmentAttrs[TCoord(sent, off).Coord].swap(*spanAttributes.SegmentAttrs);
}

void TArchiveZoneAttrs::Swap(TArchiveZoneAttrs& another) {
    DoSwap(ZoneId, another.ZoneId);
    DoSwap(Span2AttrsWrapper, another.Span2AttrsWrapper);
}

bool TArchiveZoneAttrs::Empty() const {
    return Span2AttrsWrapper.Empty();
}

void TArchiveZone::SaveAsIs(IOutputStream* out, bool saveId) const {
    if (saveId) {
        ::Save(out, ZoneId);
    }
    ::Save(out, Spans);
}

void TArchiveZone::SaveDelta(IOutputStream* out, bool saveId) const {
    if (saveId) {
        ::Save(out, ZoneId);
    }
    TVector<TArchiveZoneSpan> spans(Spans.size());
    DeltaEncodeArchiveZones(Spans, spans);
    ::Save(out, spans);
}

void TArchiveZone::LoadAsIs(IInputStream* in, bool loadId) {
    if (loadId) {
        ::Load(in, ZoneId);
    }
    ::Load(in, Spans);
}

void TArchiveZone::LoadDelta(IInputStream* in, bool loadId) {
    if (loadId) {
        ::Load(in, ZoneId);
    }
    ::Load(in, Spans);
    DeltaDecodeArchiveZones(Spans);
}

TArchiveMarkupZones::TArchiveMarkupZones()
    : Zones()
    , Attrs()
    , ArcZoneAttrVersion(CurrentArcZoneAttrVersion)
    , SegVersion(NSegm::SegSpanVersion_2)
{
    Zones.resize(AZ_COUNT);
    Attrs.resize(AZ_COUNT);
    for (size_t i = 0; i < Zones.size(); i++) {
        Zones[i].ZoneId = (ui32)i;
        Attrs[i].ZoneId = (ui32)i;
    }
}

const TArchiveZone& TArchiveMarkupZones::GetZone(ui32 id) const {
    Y_ASSERT(id < AZ_COUNT);
    return Zones[id];
}

TArchiveZone& TArchiveMarkupZones::GetZone(ui32 id) {
    Y_ASSERT(id < AZ_COUNT);
    return Zones[id];
}

TArchiveZoneAttrs& TArchiveMarkupZones::GetZoneAttrs(ui32 id) {
    Y_ASSERT(id < AZ_COUNT);
    return Attrs[id];
}

const TArchiveZoneAttrs& TArchiveMarkupZones::GetZoneAttrs(ui32 id) const {
    Y_ASSERT(id < AZ_COUNT);
    return Attrs[id];
}

typedef TVector<ui64> TUrlData;

namespace NArcPackers {
    static const NPackers::TIntegralPacker<ui64> UI64_PACKER = NPackers::TIntegralPacker<ui64>();

    class TArcPacker {
    public:
        void UnpackLeaf(const char* buf, TUrlData& data) const;
        void PackLeaf(char* buf, const TUrlData& data, size_t size) const;
        size_t MeasureLeaf(const TUrlData& data) const;
        size_t SkipLeaf(const char* buf) const;
    };

    inline void TArcPacker::UnpackLeaf(const char* buf, TUrlData& data) const {
        ui64 len;
        UI64_PACKER.UnpackLeaf(buf, len);
        buf += UI64_PACKER.SkipLeaf(buf);
        data.resize(len);
        for (ui64 i = 0; i < len; ++i) {
            UI64_PACKER.UnpackLeaf(buf, data[i]);
            buf += UI64_PACKER.SkipLeaf(buf);
        }
    }

    inline void TArcPacker::PackLeaf(char* buf, const TUrlData& data, size_t size) const {
        char* fin = buf + size;
        size_t len;

        len = UI64_PACKER.MeasureLeaf(data.size());
        UI64_PACKER.PackLeaf(buf, data.size(), len);
        buf += len;

        for (size_t i = 0; i < data.size(); ++i) {
            len = UI64_PACKER.MeasureLeaf(data[i]);
            UI64_PACKER.PackLeaf(buf, data[i], len);
            buf += len;
        }
        Y_ASSERT(buf == fin);
    }

    inline size_t TArcPacker::MeasureLeaf(const TUrlData& data) const {
        size_t len = UI64_PACKER.MeasureLeaf(data.size());
        for (size_t i = 0; i < data.size(); ++i)
            len += UI64_PACKER.MeasureLeaf(data[i]);
        return len;
    }

    inline size_t TArcPacker::SkipLeaf(const char* buf) const {
        size_t off = UI64_PACKER.SkipLeaf(buf);
        ui64 len;
        UI64_PACKER.UnpackLeaf(buf, len);
        Y_ASSERT(off == UI64_PACKER.MeasureLeaf(len));

        for (ui64 i = 0; i < len; ++i) {
            ui64 tmp;
            UI64_PACKER.UnpackLeaf(buf + off, tmp);
            const size_t skip = UI64_PACKER.SkipLeaf(buf + off);
            Y_ASSERT(skip == UI64_PACKER.MeasureLeaf(tmp));
            off += skip;
        }
        return off;
    }
}

typedef TCompactTrieHolder<wchar16, TUrlData, NArcPackers::TArcPacker> TUrlsTrieHolder;
typedef TCompactTrie<wchar16, TUrlData, NArcPackers::TArcPacker> TUrlsTrie;
typedef TUrlsTrie::TBuilder TUrlsTrieBuilder;

void TArchiveMarkupZones::SaveVersionLabel(IOutputStream* out) const {
    *out << ARC_ZONE_ATTR_VERSION_LABEL;
}

void TArchiveMarkupZones::SaveArchiveZonesAsIs(IOutputStream* out) const {
    for (size_t j = 0; j < Zones.size(); j++) {
        if (!Zones[j].Spans.empty())
            Zones[j].SaveAsIs(out);
    }
}

void TArchiveMarkupZones::SaveArchiveZonesDelta(TVector<char>* out, ui32 expectedBytes) const {
    TBufferOutput deltaTempOutput(expectedBytes);
    for (size_t j = 0; j < Zones.size(); j++) {
        if (!Zones[j].Spans.empty())
            Zones[j].SaveDelta(&deltaTempOutput);
    }
    const TBuffer& temp = deltaTempOutput.Buffer();
    NArc::GetMarkupZonesCompressor().Compress(
                TStringBuf(temp.Data(), temp.Size()),
                out);
}

void TArchiveMarkupZones::SaveArchiveZones(IOutputStream* out) const {
    ui32 zoneCount = 0;
    ui32 expectedBytes = 0;
    for (size_t i = 0; i < Zones.size(); i++) {
        if (!Zones[i].Spans.empty())
            ++zoneCount;
        expectedBytes += 4; //ZoneId
        expectedBytes += 8 + Zones[i].Spans.size() * sizeof(TArchiveZoneSpan); //Spans
    }

    TBufferOutput asisOutput(expectedBytes);
    SaveArchiveZonesAsIs(&asisOutput);
    const TBuffer& asisBuffer = asisOutput.Buffer();

    TVector<char> deltaBuffer;
    SaveArchiveZonesDelta(&deltaBuffer, asisBuffer.Size());

    if (CurrentArcZoneAttrVersion == ArcZoneAttrVersion_3 &&
        (deltaBuffer.size() + sizeof(ui32)) < asisBuffer.Size())
    {
        ::Save(out, (ui8)ArcZoneAttrVersion_3);
        ::Save(out, zoneCount);
        const ui32 size = deltaBuffer.size();
        ::Save(out, size);
        out->Write(deltaBuffer.data(), size);
    } else {
        ::Save(out, (ui8)ArcZoneAttrVersion_2);
        ::Save(out, zoneCount);
        out->Write(asisBuffer.Data(), asisBuffer.Size());
    }
}

void TArchiveMarkupZones::SaveArchiveZoneAttrs(
        IOutputStream* out,
        TMultiMap<TUtf16String, ui64>& urls) const
{
    ui32 attrsCount = 0;
    for (size_t i = 0; i < Attrs.size(); ++i) {
        if (!Attrs[i].Empty())
            ++attrsCount;
    }
    ::Save(out, attrsCount);

    for (size_t j = 0; j < Attrs.size(); ++j) {
        if (!Attrs[j].Empty())
            Attrs[j].Save(out, urls, SegVersion);
    }
}

void TArchiveMarkupZones::SaveUrlTrie(
        IOutputStream* out,
        const TMultiMap<TUtf16String, ui64>& urls) const
{
    TUrlsTrieBuilder b(CTBF_PREFIX_GROUPED);
    TUrlData temp;
    TUtf16String url;
    for (TMultiMap<TUtf16String, ui64>::const_iterator it = urls.begin(); it != urls.end(); ++it) {
        if (it == urls.begin())
            url = it->first;
        if (url != it->first) {
            b.Add(url, temp);
            url = it->first;
            temp.clear();
        }
        temp.push_back(it->second);
    }
    if (!urls.empty()) {
        b.Add(url, temp);
    }
    TBufferOutput raw;
    b.Save(raw);
    TBufferStream  min(raw.Buffer().Size());
    ui64 len = CompactTrieMinimize<TUrlsTrieBuilder::TPacker>(min, raw.Buffer().Data(), raw.Buffer().Size(), false);
    ::Save(out, len);
    TransferData(&min, out);
    ::Save(out, SegVersion);
}

void TArchiveMarkupZones::Save(IOutputStream* out) const {
    SaveVersionLabel(out);
    SaveArchiveZones(out);
    TMultiMap<TUtf16String, ui64> urls;
    SaveArchiveZoneAttrs(out, urls);
    SaveUrlTrie(out, urls);
}

void TArchiveMarkupZones::LoadAttrVersion(TMemoryInput* in) {
    size_t inputSize = in->Avail();
    const char* inputBuf = in->Buf();
    const size_t labelSize = Y_ARRAY_SIZE(ARC_ZONE_ATTR_VERSION_LABEL) - 1;

    if (inputSize < labelSize) {
        return;
    }
    char buf[labelSize];
    size_t cb = in->Load(buf, labelSize);
    if (cb != labelSize || memcmp(buf, ARC_ZONE_ATTR_VERSION_LABEL, labelSize) != 0) {
        in->Reset(inputBuf, inputSize);
        ArcZoneAttrVersion = ArcZoneAttrVersion_1;
    } else {
        ::Load(in, ArcZoneAttrVersion);
    }
}

void TArchiveMarkupZones::LoadArchiveZonesAsIs(TMemoryInput* in) {
    ui32 zoneCount = 0;
    ::Load(in, zoneCount);
    TArchiveZone temp;
    for (ui32 j = 0; j < zoneCount; j++) {
        temp.LoadAsIs(in);

        Y_ASSERT(temp.ZoneId < Zones.size());
        if (temp.ZoneId >= Zones.size())
            Zones.resize(temp.ZoneId + 1);

        Zones[temp.ZoneId].ZoneId = temp.ZoneId;
        Zones[temp.ZoneId].Spans.swap(temp.Spans);
    }
}

void TArchiveMarkupZones::LoadArchiveZonesDelta(TMemoryInput* in) {
    ui32 zoneCount = 0;
    ::Load(in, zoneCount);

    ui32 bufferSize = 0;
    ::Load(in, bufferSize);

    Y_ASSERT(in->Avail() >= bufferSize);
    if (in->Avail() < bufferSize) {
        return;
    }

    TVector<char> decompressedBuf(bufferSize * 2);
    {
        const TStringBuf compressedBuf(in->Buf(), bufferSize);
        NArc::GetMarkupZonesDecompressor().Decompress(
                    compressedBuf, &decompressedBuf);
        in->Skip(bufferSize);
    }

    TMemoryInput deltaInput(decompressedBuf.data(), decompressedBuf.size());
    TArchiveZone temp;
    for (ui32 j = 0; j < zoneCount; j++) {
        temp.LoadDelta(&deltaInput);

        Y_ASSERT(temp.ZoneId < Zones.size());
        if (temp.ZoneId >= Zones.size())
            Zones.resize(temp.ZoneId + 1);

        Zones[temp.ZoneId].ZoneId = temp.ZoneId;
        Zones[temp.ZoneId].Spans.swap(temp.Spans);
    }
}

void TArchiveMarkupZones::Load(TMemoryInput* in) {
    LoadAttrVersion(in);
    if (ArcZoneAttrVersion == ArcZoneAttrVersion_3) {
        LoadArchiveZonesDelta(in);
    } else {
        LoadArchiveZonesAsIs(in);
    }
    LoadArchiveZoneAttrs(in);
}

void TArchiveMarkupZones::LoadArchiveZoneAttrs(TMemoryInput* in) {
    if (!in->Avail()) {
        return;
    }
    Attrs.resize(Zones.size());
    ui32 attrsCount = 0;
    TArchiveZoneAttrs temp;
    ::Load(in, attrsCount);
    for (size_t i = 0; i < attrsCount; ++i) {
        temp.Load(in, ArcZoneAttrVersion);
        Y_ASSERT(temp.ZoneId < Attrs.size());
        if (temp.ZoneId >= Attrs.size())
            Attrs.resize(temp.ZoneId + 1);
        Attrs[temp.ZoneId].Swap(temp);
    }
    ui64 len;
    ::Load(in, len);
    TUrlsTrieHolder urls(*in, size_t(len));
    for (TUrlsTrie::TConstIterator it = urls.Begin(); it != urls.End(); ++it) {
        const TUrlData& data = it.GetValue();
        for (size_t i = 0; i < data.size(); ++i) {
            TArchiveZoneAttrs::TZoneCoord zc(data[i]);
            Y_ASSERT(zc.ZoneId < Attrs.size());
            if (zc.ZoneId >= Attrs.size())
                Attrs.resize(zc.ZoneId + 1);
            switch (zc.ZoneId) {
                case AZ_ANCHORINT:
                    // For now we are saving only SEGMLINKINT attributes for AZ_ANCHORINT zone.
                    // If this changes we should rewrite TUrlData representation (add info about the type of a link).
                    Attrs[zc.ZoneId].AddSpanAttr(zc.Coord, NArchiveZoneAttr::NAnchorInt::SEGMLINKINT, it.GetKey());
                    break;
                case AZ_ANCHOR:
                    Attrs[zc.ZoneId].AddSpanAttr(zc.Coord, NArchiveZoneAttr::NAnchor::LINK, it.GetKey());
                    break;
                default:
                    break;
            }
        }
    }
    SegVersion = NSegm::SegSpanVersion_1;
    if (in->Avail()) {
        ::Load(in, SegVersion);
    }
}

ui8 TArchiveMarkupZones::GetSegVersion() const {
    return SegVersion;
}

void AddExtendedBlock(
        const TBlob& origText,
        const char* block,
        size_t blSize,
        TBuffer* toWrite,
        bool replace)
{
    TBufferOutput output(*toWrite);

    TArchiveTextBlockInfo newBlockInfo;
    newBlockInfo.BlockFlag = BLOCK_IS_EXTENDED;
    newBlockInfo.EndOffset = (ui32)blSize;

    if (origText.Empty()) {
        TArchiveTextHeader hdr;
        hdr.BlockCount = 1;
        output.Write(&hdr, sizeof(hdr));
        output.Write(&newBlockInfo, sizeof(newBlockInfo));
    } else {
        const ui8* data = origText.AsUnsignedCharPtr();
        TArchiveTextHeader hdr = *((TArchiveTextHeader*)data);
        data += sizeof(TArchiveTextHeader);

        const void* zones = (const void*)data;
        data += hdr.InfoLen;

        TArchiveTextBlockInfo* blockInfos = (TArchiveTextBlockInfo*)data;
        data += hdr.BlockCount * sizeof(TArchiveTextBlockInfo);

        if (replace) {
            for (; hdr.BlockCount > 0; --hdr.BlockCount) {
                if (blockInfos[hdr.BlockCount - 1].BlockFlag != BLOCK_IS_EXTENDED)
                    break;
            }
        }

        ui32 blocksInfoLen = hdr.BlockCount * sizeof(TArchiveTextBlockInfo);
        ui32 blocksLen = 0;

        if (hdr.BlockCount > 0) {
            const TArchiveTextBlockInfo& b = blockInfos[hdr.BlockCount - 1];
            newBlockInfo.EndOffset += b.EndOffset;
            blocksLen = b.EndOffset;
        }

        hdr.BlockCount += 1;
        output.Write(&hdr, sizeof(hdr));
        output.Write(zones, hdr.InfoLen);
        output.Write(blockInfos, blocksInfoLen);
        output.Write(&newBlockInfo, sizeof(newBlockInfo));
        output.Write(data, blocksLen);
    }
    output.Write(block, blSize);
    output.Finish();
}

TString ParamFromAttrValue(const TString& v, const TString& attrName) {
    TString value = Strip(v);

    TVector<TString> params;
    StringSplitter(value).SplitBySet(ARCHIVE_FIELD_VALUE_LIST_SEP).SkipEmpty().Collect(&params);
    TString prefix = attrName + "=";

    for (TVector<TString>::iterator i = params.begin(); i != params.end(); ++i) {
        TString& param = *i;

        if (param.StartsWith(prefix))
            return param.substr(prefix.length());
    }

    return "";
}

TString ParamFromArcHeader(const THashMap<TString, TString>& headers, const TString& ns, const TString& attrName) {
    THashMap<TString, TString>::const_iterator i = headers.find(ns);
    if (i != headers.end()) {
        return ParamFromAttrValue(i->second, attrName);
    }
    return "";
}
