#include "chunkslist.h"

#include <library/cpp/html/face/blob/chunkslist.pb.h>

#include <library/cpp/html/face/event.h>
#include <library/cpp/html/spec/attrs.h>

#include <util/generic/vector.h>
#include <util/memory/pool.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite.h>

namespace NHtml {
    ///////////////////////////////////////////////////////////////////////////////

    using namespace google::protobuf::io;
    using namespace google::protobuf::internal;

    ///////////////////////////////////////////////////////////////////////////////

    class THtmlChunksWriter::TImpl {
        class TUnalignedPool: protected  TMemoryPool {
        public:
            explicit TUnalignedPool(size_t initial)
                : TMemoryPool(initial)
            {
            }

            inline ui8* AllocateChunk(size_t len) {
                return (ui8*)TMemoryPool::RawAllocate(len);
            }
        };

    public:
        TImpl(size_t docLen)
            : Memory_(Max(size_t(64) << 10, docLen << 2))
            , Total_(0)
        {
            Chunks_.push_back(std::make_pair(Memory_.AllocateChunk(0), 0));
        }

        void OnHtmlChunk(const THtmlChunk& chunk) {
            const ui32 size = ChunkByteSize(chunk);
            const ui32 leng = 1 + WireFormatLite::LengthDelimitedSize(size);
            ui8* data = Memory_.AllocateChunk(leng);
            ui8* tmp = WriteChunk(chunk, size, data);
            Y_ASSERT(ui32(tmp - data) == leng);
            if (Chunks_.back().first + Chunks_.back().second == data) {
                Chunks_.back().second += leng;
            } else {
                Chunks_.push_back(std::make_pair(data, leng));
            }
            Total_ += leng;
        }

        TBuffer CreateResultBuffer() {
            OnHtmlChunk(THtmlChunk(PARSED_EOF));

            TBuffer buf(Total_);
            for (size_t i = 0; i < Chunks_.size(); ++i) {
                buf.Append((const char*)Chunks_[i].first, Chunks_[i].second);
            }
            return buf;
        }

    private:
        static inline ui8* WriteBytesField(const int field, const ui8* data, ui32 len, ui8* output) {
            output = CodedOutputStream::WriteVarint32ToArray(
                WireFormatLite::MakeTag(field, WireFormatLite::WIRETYPE_LENGTH_DELIMITED), output);
            output = CodedOutputStream::WriteVarint32ToArray(len, output);
            output = CodedOutputStream::WriteRawToArray(data, len, output);
            return output;
        }

        static inline ui8* WriteRecord(const int field, ui32 size, ui8* output) {
            output = CodedOutputStream::WriteVarint32ToArray(
                WireFormatLite::MakeTag(field, WireFormatLite::WIRETYPE_LENGTH_DELIMITED), output);
            output = CodedOutputStream::WriteVarint32ToArray(size, output);
            return output;
        }

        static inline ui64 ParsedFlagsToUi64(PARSED_FLAGS flags) {
            static_assert(sizeof(PARSED_FLAGS) == sizeof(ui64), "not equal to ui64");
            return *reinterpret_cast<const ui64*>(&flags);
        }

        static inline ui32 ExtentByteSize(const NHtml::TExtent& range) {
            return CodedOutputStream::VarintSize32(range.Start) + CodedOutputStream::VarintSize32(range.Leng) + 2;
        }

        static ui32 AttributeByteSize(const NHtml::TAttribute& attr) {
            ui32 size = 0;
            // optional TRange Name = 1;
            size += 1 + WireFormatLite::LengthDelimitedSize(ExtentByteSize(attr.Name));
            if (!attr.IsBoolean()) {
                // optional TRange Value = 2;
                size += 1 + WireFormatLite::LengthDelimitedSize(ExtentByteSize(attr.Value));
                // optional uint32 Quote = 3;
                if (attr.Quot != '\0') {
                    size += 1 + 1;
                }
            }
            // optional uint32 Namespace = 4;
            return size;
        }

        static ui32 ChunkByteSize(const THtmlChunk& chunk) {
            ui32 size = 1 + sizeof(ui64) + 1 + CodedOutputStream::VarintSize32(chunk.Format);

            switch (chunk.GetLexType()) {
                case HTLEX_START_TAG:
                case HTLEX_EMPTY_TAG:
                    // repeated TAttribute Attributes = 8;
                    for (size_t i = 0; i < chunk.AttrCount; ++i) {
                        size += 1 + WireFormatLite::LengthDelimitedSize(AttributeByteSize(chunk.Attrs[i]));
                    }
                    [[fallthrough]];

                case HTLEX_END_TAG:
                    if (chunk.text && chunk.leng) {
                        // optional bytes Text = 3;
                        size += 1 + WireFormatLite::LengthDelimitedSize(chunk.leng);
                    }
                    if (chunk.Tag) {
                        // optional uint32 Tag = 4;
                        size += 1 + CodedOutputStream::VarintSize32(chunk.Tag->id());
                    }
                    break;

                case HTLEX_TEXT:
                    if (chunk.IsCDATA) {
                        // optional bool IsCDATA = 5;
                        size += 1 + 1;
                    }
                    if (chunk.IsWhitespace) {
                        // optional bool IsWhitespace = 6;
                        size += 1 + 1;
                    }
                    [[fallthrough]];

                case HTLEX_MD:
                case HTLEX_PI:
                case HTLEX_ASP:
                case HTLEX_COMMENT:
                    if (chunk.text && chunk.leng) {
                        // optional bytes Text = 3;
                        size += 1 + WireFormatLite::LengthDelimitedSize(chunk.leng);
                    }
                    if (chunk.Tag) {
                        // optional uint32 Tag = 4;
                        size += 1 + CodedOutputStream::VarintSize32(chunk.Tag->id());
                    }
                    break;

                case HTLEX_EOF:
                    break;
            }
            return size;
        }

        static inline ui8* WriteExtent(const NHtml::TExtent& range, ui8* p) {
            p = WireFormatLite::WriteInt32ToArray(THtmlChunksList::TRange::kOffsetFieldNumber, range.Start, p);
            p = WireFormatLite::WriteInt32ToArray(THtmlChunksList::TRange::kLengthFieldNumber, range.Leng, p);
            return p;
        }

        static ui8* WriteChunk(const THtmlChunk& chunk, ui32 size, ui8* p) {
            p = WriteRecord(1, size, p);

            // required fixed64 Flags = 1;
            p = WireFormatLite::WriteFixed64ToArray(1, ParsedFlagsToUi64(chunk.flags), p);
            // optional uint32 Format = 2;
            p = WireFormatLite::WriteUInt32ToArray(2, chunk.Format, p);

            switch (chunk.GetLexType()) {
                case HTLEX_START_TAG:
                case HTLEX_EMPTY_TAG:
                    // repeated TAttribute Attributes = 8;
                    for (size_t i = 0; i < chunk.AttrCount; ++i) {
                        const NHtml::TAttribute& attr = chunk.Attrs[i];

                        p = WriteRecord(8, AttributeByteSize(attr), p);

                        // optional TRange Name = 1;
                        p = WriteRecord(1, ExtentByteSize(attr.Name), p);
                        p = WriteExtent(attr.Name, p);

                        if (!attr.IsBoolean()) {
                            // optional TRange Value = 2;
                            p = WriteRecord(2, ExtentByteSize(attr.Value), p);
                            p = WriteExtent(attr.Value, p);
                            // optional uint32 Quote = 3;
                            if (attr.Quot != '\0') {
                                p = WireFormatLite::WriteUInt32ToArray(3, attr.Quot, p);
                            }
                        }

                        // optional uint32 Namespace = 4;
                    }
                    [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
                case HTLEX_END_TAG:
                    if (chunk.text && chunk.leng) {
                        // optional bytes Text = 3;
                        p = WriteBytesField(3, (const ui8*)chunk.text, chunk.leng, p);
                    }
                    if (chunk.Tag) {
                        // optional uint32 Tag = 4;
                        p = WireFormatLite::WriteUInt32ToArray(4, chunk.Tag->id(), p);
                    }
                    break;

                case HTLEX_TEXT:
                    if (chunk.IsCDATA) {
                        // optional bool IsCDATA = 5;
                        p = WireFormatLite::WriteBoolToArray(5, chunk.IsCDATA, p);
                    }
                    if (chunk.IsWhitespace) {
                        // optional bool IsWhitespace = 6;
                        p = WireFormatLite::WriteBoolToArray(6, chunk.IsWhitespace, p);
                    }
                    [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
                case HTLEX_MD:
                case HTLEX_PI:
                case HTLEX_ASP:
                case HTLEX_COMMENT:
                    if (chunk.text && chunk.leng) {
                        // optional bytes Text = 3;
                        p = WriteBytesField(3, (const ui8*)chunk.text, chunk.leng, p);
                    }
                    if (chunk.Tag) {
                        // optional uint32 Tag = 4;
                        p = WireFormatLite::WriteUInt32ToArray(4, chunk.Tag->id(), p);
                    }
                    break;

                case HTLEX_EOF:
                    break;
            }
            return p;
        }

    private:
        TUnalignedPool Memory_;
        TVector<std::pair<ui8*, ui32>> Chunks_;
        ui32 Total_;
    };

    ///////////////////////////////////////////////////////////////////////////////

    THtmlChunksWriter::THtmlChunksWriter(size_t docLen)
        : Impl_(new TImpl(docLen))
    {
    }

    THtmlChunksWriter::~THtmlChunksWriter() {
    }

    TBuffer THtmlChunksWriter::CreateResultBuffer() {
        return Impl_->CreateResultBuffer();
    }

    THtmlChunk* THtmlChunksWriter::OnHtmlChunk(const THtmlChunk& chunk) {
        Impl_->OnHtmlChunk(chunk);
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////

    class THtmlChunksReader {
        typedef TVector<NHtml::TAttribute> TAttributeVector;
        typedef CodedInputStream::Limit TLimit;

    public:
        THtmlChunksReader(const char* data, size_t len)
            : Data_(data)
            , Len_(len)
        {
        }

        bool NumerateChunks(IParserResult* result) {
            CodedInputStream input(reinterpret_cast<const ui8*>(Data_), Len_);
            TAttributeVector attributes;
            ui32 tag;

            while ((tag = input.ReadTag()) != 0) {
                switch (WireFormatLite::GetTagFieldNumber(tag)) {
                    case THtmlChunksList::kChunksFieldNumber: {
                        ui32 size;
                        if (Y_UNLIKELY(!input.ReadVarint32(&size))) {
                            return false;
                        }
                        //
                        // Read chunk
                        //
                        THtmlChunk chunk(PARSED_ERROR);
                        TLimit limit = input.PushLimit(size);
                        if (!ReadChunk(&input, &chunk, &attributes)) {
                            return false;
                        }
                        input.PopLimit(limit);
                        //
                        // Emit chunk
                        //
                        if (!attributes.empty()) {
                            chunk.Attrs = attributes.data();
                            chunk.AttrCount = attributes.size();
                        }
                        result->OnHtmlChunk(chunk);
                        attributes.clear();
                        break;
                    }

                    default: {
                        if (Y_UNLIKELY(!WireFormatLite::SkipField(&input, tag))) {
                            return false;
                        }
                    }
                }
            }

            return input.CurrentPosition() == ssize_t(Len_);
        }

    private:
        bool ReadExtent(CodedInputStream* input, NHtml::TExtent* range) {
            ui32 tag;
            ui32 bitmask = 0;
            while ((tag = input->ReadTag()) != 0) {
                switch (WireFormatLite::GetTagFieldNumber(tag)) {
                    case THtmlChunksList::TRange::kOffsetFieldNumber: {
                        if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &range->Start)) {
                            return false;
                        }
                        bitmask |= 0x01;
                        break;
                    }

                    case THtmlChunksList::TRange::kLengthFieldNumber: {
                        if (!WireFormatLite::ReadPrimitive<ui32, WireFormatLite::TYPE_UINT32>(input, &range->Leng)) {
                            return false;
                        }
                        bitmask |= 0x02;
                        break;
                    }

                    default: {
                        if (Y_UNLIKELY(!WireFormatLite::SkipField(input, tag))) {
                            return false;
                        }
                    }
                }
            }
            return ((bitmask & 0x01) != 0) && ((bitmask & 0x02) != 0);
        }

        bool ReadAttribute(CodedInputStream* input, NHtml::TAttribute* attribute) {
            ui32 tag;
            ui32 bitmask = 0;
            while ((tag = input->ReadTag()) != 0) {
                switch (WireFormatLite::GetTagFieldNumber(tag)) {
                    case THtmlChunksList::TAttribute::kNameFieldNumber: {
                        ui32 size = 0;
                        if (Y_UNLIKELY(!input->ReadVarint32(&size))) {
                            return false;
                        }
                        TLimit limit = input->PushLimit(size);
                        {
                            NHtml::TExtent range;
                            if (!ReadExtent(input, &range)) {
                                return false;
                            }
                            attribute->Name = range;
                            bitmask |= 0x01;
                        }
                        input->PopLimit(limit);
                        break;
                    }

                    case THtmlChunksList::TAttribute::kValueFieldNumber: {
                        ui32 size = 0;
                        if (Y_UNLIKELY(!input->ReadVarint32(&size))) {
                            return false;
                        }
                        TLimit limit = input->PushLimit(size);
                        {
                            NHtml::TExtent range;
                            if (!ReadExtent(input, &range)) {
                                return false;
                            }
                            attribute->Value = range;
                            bitmask |= 0x02;
                        }
                        input->PopLimit(limit);
                        break;
                    }

                    case THtmlChunksList::TAttribute::kQuoteFieldNumber: {
                        ui32 quote = 0;
                        if (!input->ReadVarint32(&quote)) {
                            return false;
                        }
                        attribute->Quot = (char)quote;
                        break;
                    }

                    case THtmlChunksList::TAttribute::kNamespaceFieldNumber: {
                        ui32 ns = 0;
                        if (!input->ReadVarint32(&ns)) {
                            return false;
                        }
                        attribute->Namespace = EAttrNS(ns);
                        break;
                    }

                    default: {
                        if (Y_UNLIKELY(!WireFormatLite::SkipField(input, tag))) {
                            return false;
                        }
                    }
                }
            }
            // Attribute without value is so called 'boolean attribute' - Value must be equal to Name.
            if ((bitmask & 0x02) == 0) {
                attribute->Value = attribute->Name;
            }
            return (bitmask & 0x01) != 0;
        }

        bool ReadChunk(CodedInputStream* input, THtmlChunk* chunk, TAttributeVector* attributes) {
            ui32 tag;
            while ((tag = input->ReadTag()) != 0) {
                switch (WireFormatLite::GetTagFieldNumber(tag)) {
                    case THtmlChunksList::TChunk::kFlagsFieldNumber: {
                        static_assert(sizeof(chunk->flags) == sizeof(ui64), "not equal to ui64");
                        ui64 flags = 0;
                        if (!WireFormatLite::ReadPrimitive<ui64, WireFormatLite::TYPE_FIXED64>(input, &flags)) {
                            return false;
                        } else {
                            memcpy(&chunk->flags, &flags, sizeof(chunk->flags));
                        }
                        if (input->ExpectTag(WireFormatLite::MakeTag(THtmlChunksList::TChunk::kFormatFieldNumber, WireFormatLite::WIRETYPE_VARINT))) {
                            goto parse_Format;
                        }
                        break;
                    }

                    case THtmlChunksList::TChunk::kFormatFieldNumber: {
                    parse_Format:
                        ui32 format = 0;
                        if (!input->ReadVarint32(&format)) {
                            return false;
                        } else {
                            chunk->Format = static_cast<TIrregTag>(format);
                        }
                        if (input->ExpectTag(WireFormatLite::MakeTag(THtmlChunksList::TChunk::kTextFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED))) {
                            goto parse_Text;
                        }
                        break;
                    }

                    case THtmlChunksList::TChunk::kTextFieldNumber: {
                    parse_Text:
                        ui32 textLen = 0;
                        if (Y_UNLIKELY(!input->ReadVarint32(&textLen))) {
                            return false;
                        }
                        // save pointer to current string
                        const char* text = Data_ + input->CurrentPosition();
                        if (!input->Skip(textLen)) {
                            return false;
                        }

                        chunk->text = text;
                        chunk->leng = textLen;

                        if (input->ExpectTag(WireFormatLite::MakeTag(THtmlChunksList::TChunk::kTagFieldNumber, WireFormatLite::WIRETYPE_VARINT))) {
                            goto parse_Tag;
                        }
                        break;
                    }

                    case THtmlChunksList::TChunk::kTagFieldNumber: {
                    parse_Tag:
                        ui32 tagid = 0;
                        if (!input->ReadVarint32(&tagid)) {
                            return false;
                        }
                        chunk->Tag = &NHtml::FindTag(static_cast<HT_TAG>(tagid));
                        if (input->ExpectTag(WireFormatLite::MakeTag(THtmlChunksList::TChunk::kAttributesFieldNumber, WireFormatLite::WIRETYPE_LENGTH_DELIMITED))) {
                            goto parse_Attributes;
                        }
                        break;
                    }

                    case THtmlChunksList::TChunk::kAttributesFieldNumber: {
                    parse_Attributes:
                        ui32 size = 0;
                        if (Y_UNLIKELY(!input->ReadVarint32(&size))) {
                            return false;
                        }
                        NHtml::TAttribute attr;
                        TLimit limit = input->PushLimit(size);
                        if (!ReadAttribute(input, &attr)) {
                            return false;
                        }
                        attributes->push_back(attr);
                        input->PopLimit(limit);
                        break;
                    }

                    case THtmlChunksList::TChunk::kIsCDATAFieldNumber: {
                        if (!WireFormatLite::ReadPrimitive<bool, WireFormatLite::TYPE_BOOL>(input, &chunk->IsCDATA)) {
                            return false;
                        }
                        break;
                    }

                    case THtmlChunksList::TChunk::kIsWhitespaceFieldNumber: {
                        if (!WireFormatLite::ReadPrimitive<bool, WireFormatLite::TYPE_BOOL>(input, &chunk->IsWhitespace)) {
                            return false;
                        }
                        break;
                    }

                    case THtmlChunksList::TChunk::kNamespaceFieldNumber: {
                        ui32 ns = 0;
                        if (!input->ReadVarint32(&ns)) {
                            return false;
                        }
                        chunk->Namespace = ETagNamespace(ns);
                        break;
                    }

                    default: {
                        if (Y_UNLIKELY(!WireFormatLite::SkipField(input, tag))) {
                            return false;
                        }
                    }
                }
            }

            return true;
        }

    private:
        const char* const Data_;
        const size_t Len_;
    };

    ///////////////////////////////////////////////////////////////////////////////

    bool NumerateHtmlChunks(const TChunksRef& chunks, IParserResult* result) {
        if (chunks.empty()) {
            return true;
        }
        return THtmlChunksReader(chunks.data(), chunks.size()).NumerateChunks(result);
    }

    ///////////////////////////////////////////////////////////////////////////////

}
