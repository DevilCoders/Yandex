#include <library/cpp/testing/gtest/gtest.h>

#include <library/cpp/xdelta3/proto/state_header.pb.h>
#include <library/cpp/xdelta3/state/create_proto.h>
#include <library/cpp/xdelta3/state/data_ptr.h>
#include <library/cpp/xdelta3/state/hash.h>
#include <library/cpp/xdelta3/state/merge.h>
#include <library/cpp/xdelta3/state/state.h>
#include <library/cpp/xdelta3/xdelta_codec/codec.h>

#include <google/protobuf/arena.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/util/message_differencer.h>

using namespace NXdeltaAggregateColumn;

static const ui8* ByteBuffer(const char* ptr) {
    return reinterpret_cast<const ui8*>(ptr);
}

static bool ParseHeader(const ui8* data, TStateHeader& header)
{
    using namespace NProtoBuf::io;
    auto headerSize = data[0];
    ArrayInputStream array(data + sizeof(headerSize), headerSize);
    CodedInputStream istream(&array);
    return header.ParseFromCodedStream(&istream);
}

TEST(XDeltaMergeProtoHeader, MissingRequiredBaseHashField)
{
    auto data = "12345";
    auto dataSize = strlen(data);
    auto patch = "12345777";
    auto patchSize = strlen(data);
    auto buffer = EncodePatchProtoAsBuffer(
        ByteBuffer(data),
        dataSize,
        ByteBuffer(patch),
        patchSize,
        [](auto& hdr) { hdr.clear_base_hash(); });
    NProtoBuf::Arena arena;
    TState proto(arena, ByteBuffer(buffer.Data()), buffer.Size());
    EXPECT_EQ(TStateHeader::NONE_TYPE, proto.Type());
    EXPECT_EQ(TStateHeader::MISSING_REQUIRED_FIELD_ERROR, proto.Error());
}

TEST(XDeltaMergeProtoHeader, MissingRequiredStateHashField)
{
    auto data = "12345";
    auto dataSize = strlen(data);
    auto patch = "12345777";
    auto patchSize = strlen(data);
    auto buffer = EncodePatchProtoAsBuffer(
        ByteBuffer(data),
        dataSize,
        ByteBuffer(patch),
        patchSize,
        [](auto& hdr) { hdr.clear_state_hash(); });
    NProtoBuf::Arena arena;
    TState proto(arena, ByteBuffer(buffer.Data()), buffer.Size());
    EXPECT_EQ(TStateHeader::NONE_TYPE, proto.Type());
    EXPECT_EQ(TStateHeader::MISSING_REQUIRED_FIELD_ERROR, proto.Error());
}

TEST(XDeltaMergeProtoHeader, MissingRequiredPatchDataSizeField)
{
    auto data = "12345";
    auto dataSize = strlen(data);
    auto patch = "12345777";
    auto patchSize = strlen(data);
    auto buffer = EncodePatchProtoAsBuffer(
        ByteBuffer(data),
        dataSize,
        ByteBuffer(patch),
        patchSize,
        [](auto& hdr) { hdr.clear_data_size(); });
    NProtoBuf::Arena arena;
    TState proto(arena, ByteBuffer(buffer.Data()), buffer.Size());
    EXPECT_EQ(TStateHeader::NONE_TYPE, proto.Type());
    EXPECT_EQ(TStateHeader::MISSING_REQUIRED_FIELD_ERROR, proto.Error());
}

TEST(XDeltaMergeProtoHeader, MissingRequiredErrorCodeField)
{
    auto buffer = EncodeErrorProtoAsBuffer(
        TStateHeader::MISSING_REQUIRED_FIELD_ERROR,
        [](auto& hdr) { hdr.clear_error_code(); });
    NProtoBuf::Arena arena;
    TState proto(arena, ByteBuffer(buffer.Data()), buffer.Size());
    EXPECT_EQ(TStateHeader::NONE_TYPE, proto.Type());
    EXPECT_EQ(TStateHeader::MISSING_REQUIRED_FIELD_ERROR, proto.Error());
}

TEST(XDeltaMergeProtoHeader, MissingRequiredBaseDataSizeField)
{
    auto data = "12345";
    auto dataSize = strlen(data);
    auto buffer = EncodeBaseProtoAsBuffer(
        ByteBuffer(data),
        dataSize,
        [](auto& hdr) { hdr.clear_data_size(); });
    NProtoBuf::Arena arena;
    TState proto(arena, ByteBuffer(buffer.Data()), buffer.Size());
    EXPECT_EQ(TStateHeader::NONE_TYPE, proto.Type());
    EXPECT_EQ(TStateHeader::MISSING_REQUIRED_FIELD_ERROR, proto.Error());
}

TEST(XDeltaMergeProtoHeader, BaseWrongDataSize) {
    auto data = "12345";
    auto dataSize = strlen(data);
    auto buffer = EncodeBaseProtoAsBuffer(
        ByteBuffer(data),
        dataSize,
        [](auto& hdr) { hdr.set_data_size(hdr.data_size() + 100); });
    NProtoBuf::Arena arena;
    TState proto(arena, ByteBuffer(buffer.Data()), buffer.Size());
    EXPECT_EQ(TStateHeader::NONE_TYPE, proto.Type());
    EXPECT_EQ(TStateHeader::WRONG_DATA_SIZE, proto.Error());
}

TEST(XDeltaMergeProtoHeader, WrongHeaderSizeLarger)
{
    auto data = "12345";
    auto dataSize = strlen(data);
    auto buffer = EncodeBaseProtoAsBuffer(ByteBuffer(data), dataSize);
    buffer.Data()[0] += dataSize;
    NProtoBuf::Arena arena;
    TState proto(arena, ByteBuffer(buffer.Data()), buffer.Size());
    EXPECT_EQ(TStateHeader::NONE_TYPE, proto.Type());
    EXPECT_EQ(TStateHeader::HEADER_PARSE_ERROR, proto.Error());
    EXPECT_EQ(0ull, proto.PayloadSize());
    EXPECT_EQ(nullptr, proto.PayloadData());
}

TEST(XDeltaMergeProtoHeader, WrongHeaderSizeSmaller)
{
    auto data = "12345";
    auto dataSize = strlen(data);
    auto buffer = EncodeBaseProtoAsBuffer(ByteBuffer(data), dataSize);
    --buffer.Data()[0];
    NProtoBuf::Arena arena;
    TState proto(arena, ByteBuffer(buffer.Data()), buffer.Size());
    EXPECT_EQ(TStateHeader::NONE_TYPE, proto.Type());
    EXPECT_EQ(TStateHeader::HEADER_PARSE_ERROR, proto.Error());
}

TEST(XDeltaMergeProtoHeader, CodedOutputStreamBounds)
{
    using namespace NProtoBuf::io;

    auto data = "12345";
    auto dataSize = strlen(data);
    auto buffer = EncodeBaseProtoAsBuffer(ByteBuffer(data), dataSize);
    NProtoBuf::Arena arena;
    auto state = TState(arena, (ui8*)buffer.Data(), buffer.Size());

    ui8 output[1];
    ArrayOutputStream array(output, sizeof(output));
    CodedOutputStream out(&array);
    EXPECT_FALSE(state.Header().SerializeToCodedStream(&out));
}

TEST(XDeltaMergeProtoHeader, WrongHeaderSizeLargerThanDataSize)
{
    auto data = "12345";
    auto dataSize = strlen(data);
    auto buffer = EncodeBaseProtoAsBuffer(
        ByteBuffer(data),
        dataSize,
        [](auto& hdr) { hdr.set_data_size(hdr.data_size() + 10); });
    NProtoBuf::Arena arena;
    TState proto(arena, ByteBuffer(buffer.Data()), buffer.Size());
    EXPECT_EQ(TStateHeader::NONE_TYPE, proto.Type());
    EXPECT_EQ(TStateHeader::WRONG_DATA_SIZE, proto.Error());
}

TEST(XDeltaMergeProtoHeader, EmptyIsWrong)
{
    TStateHeader empty;
    auto encoded = EncodeProtoAsBuffer(empty, nullptr, 0);
    NProtoBuf::Arena arena;
    TState proto(arena, ByteBuffer(encoded.Data()), encoded.Size());
    EXPECT_EQ(TStateHeader::NONE_TYPE, proto.Type());
}

TEST(XDeltaMergeProtoHeader, ErrorHasNoPayload)
{
    auto error = EncodeErrorProtoAsBuffer(TStateHeader::MERGE_PATCHES_ERROR);
    NProtoBuf::Arena arena;
    TState proto(arena, ByteBuffer(error.Data()), error.Size());
    EXPECT_EQ(TStateHeader::NONE_TYPE, proto.Type());
    EXPECT_EQ(nullptr, proto.PayloadData());
    EXPECT_EQ(0ull, proto.PayloadSize());
}

TEST(XDeltaMergeProtoHeader, EmptyBase)
{
    auto encoded = EncodeBaseProtoAsBuffer(ByteBuffer(""), 0);
    NProtoBuf::Arena arena;
    TState proto(arena, ByteBuffer(encoded.Data()), encoded.Size());
    EXPECT_EQ(TStateHeader::BASE, proto.Type());
    EXPECT_FALSE(proto.PayloadData());
    EXPECT_FALSE(proto.PayloadSize());
}

TEST(XDeltaMergeProtoHeader, EmptyPatch)
{
    auto base = "12345";
    auto baseSize = strlen(base);
    auto state = "123456";
    auto stateSize = strlen(state);
    auto encoded = EncodePatchProtoAsBuffer(ByteBuffer(base), baseSize, ByteBuffer(state), stateSize, nullptr, 0);
    NProtoBuf::Arena arena;
    TState proto(arena, ByteBuffer(encoded.Data()), encoded.Size());
    EXPECT_EQ(TStateHeader::PATCH, proto.Type());
    EXPECT_FALSE(proto.PayloadData());
    EXPECT_FALSE(proto.PayloadSize());
}


static void Free(const TSpan& span)
{
    free(const_cast<ui8*>(span.Data));
}

TEST(XDeltaMergeProto, ApplyPatch)
{
    auto base = "12345";
    auto baseSize = strlen(base);
    auto state = "12243465";
    auto stateSize = strlen(state);

    auto baseProto = EncodeBaseProtoAsBuffer(ByteBuffer(base), baseSize);
    auto patchProto = EncodePatchProtoAsBuffer(ByteBuffer(base), baseSize, ByteBuffer(state), stateSize);
    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(baseProto.Data()), baseProto.Size(), ByteBuffer(patchProto.Data()), patchProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    TStateHeader header;
    EXPECT_TRUE(ParseHeader(result.Data + result.Offset, header));
    EXPECT_TRUE(header.type() == TStateHeader::BASE);
    EXPECT_EQ(stateSize, header.data_size());
    EXPECT_EQ(0, memcmp(state, result.Data + result.Offset + SizeOfHeader(header), stateSize));

    Free(result);
}

TEST(XDeltaMergeProto, ApplyPatchEmptyBase)
{
    auto base = "";
    auto baseSize = strlen(base);
    auto state = "12243465";
    auto stateSize = strlen(state);

    auto baseProto = EncodeBaseProtoAsBuffer(ByteBuffer(base), baseSize);
    auto patchProto = EncodePatchProtoAsBuffer(ByteBuffer(base), baseSize, ByteBuffer(state), stateSize);
    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(baseProto.Data()), baseProto.Size(), ByteBuffer(patchProto.Data()), patchProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    TStateHeader header;
    EXPECT_TRUE(ParseHeader(result.Data + result.Offset, header));
    EXPECT_EQ(TStateHeader::BASE, header.type());
    EXPECT_EQ(stateSize, header.data_size());
    EXPECT_EQ(0, memcmp(state, result.Data + result.Offset + SizeOfHeader(header), stateSize));

    Free(result);
}

TEST(XDeltaMergeProto, ApplyPatchEmptyTarget)
{
    auto base = "12243465";
    auto baseSize = strlen(base);
    auto state = "";
    auto stateSize = strlen(state);

    auto baseProto = EncodeBaseProtoAsBuffer(ByteBuffer(base), baseSize);
    auto patchProto = EncodePatchProtoAsBuffer(ByteBuffer(base), baseSize, ByteBuffer(state), stateSize);

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(baseProto.Data()), baseProto.Size(), ByteBuffer(patchProto.Data()), patchProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    TStateHeader header;
    EXPECT_TRUE(ParseHeader(result.Data + result.Offset, header));
    EXPECT_EQ(TStateHeader::BASE, header.type());
    EXPECT_EQ(0ull, header.data_size());

    Free(result);
}

TEST(XDeltaMergeProto, ApplyEmptyPatch)
{
    auto base = "12345";
    auto baseSize = strlen(base);
    auto state = "12345";
    auto stateSize = strlen(state);
    auto baseProto = EncodeBaseProtoAsBuffer(ByteBuffer(base), baseSize);
    auto patchProto = EncodePatchProtoAsBuffer(ByteBuffer(base), baseSize, ByteBuffer(state), stateSize, nullptr, 0);

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(baseProto.Data()), baseProto.Size(), ByteBuffer(patchProto.Data()), patchProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    TStateHeader header;
    EXPECT_TRUE(ParseHeader(result.Data + result.Offset, header));
    EXPECT_EQ(TStateHeader::BASE, header.type());
    EXPECT_EQ(0, memcmp(state, result.Data + result.Offset + SizeOfHeader(header), stateSize));

    Free(result);
}

TEST(XDeltaMergeProto, ApplyEmptyPatchStateSizeMismatch)
{
    auto base = "12345";
    auto baseSize = strlen(base);
    auto state = "123456";
    auto stateSize = strlen(state);
    auto baseProto = EncodeBaseProtoAsBuffer(ByteBuffer(base), baseSize);
    auto patchProto = EncodePatchProtoAsBuffer(ByteBuffer(base), baseSize, ByteBuffer(state), stateSize, nullptr, 0);

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(baseProto.Data()), baseProto.Size(), ByteBuffer(patchProto.Data()), patchProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    TStateHeader header;
    EXPECT_TRUE(ParseHeader(result.Data + result.Offset, header));
    EXPECT_EQ(TStateHeader::NONE_TYPE, header.type());
    EXPECT_TRUE(header.has_error_code());
    EXPECT_EQ(TStateHeader::STATE_SIZE_ERROR, header.error_code());

    Free(result);
}

TEST(XDeltaMergeProto, ApplyEmptyBaseEmptyPatch)
{
    auto baseProto = EncodeBaseProtoAsBuffer(nullptr, 0);
    auto patchProto = EncodePatchProtoAsBuffer(nullptr, 0, nullptr, 0, nullptr, 0);

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(baseProto.Data()), baseProto.Size(), ByteBuffer(patchProto.Data()), patchProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    TStateHeader header;
    EXPECT_TRUE(ParseHeader(result.Data + result.Offset, header));
    EXPECT_EQ(TStateHeader::BASE, header.type());
    EXPECT_EQ(0ull, header.data_size());

    Free(result);
}

TEST(XDeltaMergeProto, MergeEmptyPatchEmptyPatch)
{
    auto patch1Proto = EncodePatchProtoAsBuffer(nullptr, 0, nullptr, 0, nullptr, 0);
    auto patch2Proto = EncodePatchProtoAsBuffer(nullptr, 0, nullptr, 0, nullptr, 0);

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(patch1Proto.Data()), patch1Proto.Size(), ByteBuffer(patch2Proto.Data()), patch2Proto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    TStateHeader header;
    EXPECT_TRUE(ParseHeader(result.Data + result.Offset, header));
    EXPECT_EQ(TStateHeader::PATCH, header.type());
    EXPECT_EQ(0ull, header.data_size());

    TStateHeader patch1;
    ParseHeader(ByteBuffer(patch1Proto.Data()), patch1);
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(patch1, header));

    Free(result);
}

TEST(XDeltaMergeProto, MergeEmptyPatchEmptyPatchWrongStateHash)
{
    auto patch1Proto = EncodePatchProtoAsBuffer(nullptr, 0, nullptr, 0, nullptr, 0);
    auto patch2Proto = EncodePatchProtoAsBuffer(
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        [](auto& hdr) { hdr.set_state_hash(1); });

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(patch1Proto.Data()), patch1Proto.Size(), ByteBuffer(patch2Proto.Data()), patch2Proto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    TStateHeader header;
    EXPECT_TRUE(ParseHeader(result.Data + result.Offset, header));
    EXPECT_EQ(TStateHeader::NONE_TYPE, header.type());
    EXPECT_TRUE(header.has_error_code());
    EXPECT_EQ(TStateHeader::MERGE_PATCHES_ERROR, header.error_code());

    Free(result);
}

TEST(XDeltaMergeProto, MergeNonEmptyPatchEmptyPatch)
{
    auto base = "";
    auto baseSize = 0;
    auto state = "123456";
    auto stateSize = strlen(state);
    auto patchProto = EncodePatchProtoAsBuffer(ByteBuffer(base), baseSize, ByteBuffer(state), stateSize);
    auto hash = CalcHash(ByteBuffer(state), stateSize);
    auto emptyPatchProto = EncodePatchProtoAsBuffer(
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        [hash](auto& hdr) { hdr.set_base_hash(hash); hdr.set_state_hash(hash); });

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(patchProto.Data()), patchProto.Size(), ByteBuffer(emptyPatchProto.Data()), emptyPatchProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    TStateHeader header;
    EXPECT_TRUE(ParseHeader(result.Data + result.Offset, header));
    EXPECT_EQ(TStateHeader::PATCH, header.type());

    TStateHeader patch1;
    ParseHeader(ByteBuffer(patchProto.Data()), patch1);
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(patch1, header));
    EXPECT_TRUE(header.data_size());
    EXPECT_EQ(0, memcmp(result.Data + result.Offset + SizeOfHeader(header), patchProto.Data() + SizeOfHeader(patch1), header.data_size()));

    Free(result);
}

TEST(XDeltaMergeProto, MergeNonEmptyPatchEmptyPatchHashMismatch)
{
    auto base = "";
    auto baseSize = 0;
    auto state = "123456";
    auto stateSize = strlen(state);
    auto patchProto = EncodePatchProtoAsBuffer(ByteBuffer(base), baseSize, ByteBuffer(state), stateSize);
    auto hash = CalcHash(ByteBuffer(state), stateSize);
    auto emptyPatchProto = EncodePatchProtoAsBuffer(
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        [hash](auto& hdr) { hdr.set_base_hash(hash); hdr.set_state_hash(hash + 1); });

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(patchProto.Data()), patchProto.Size(), ByteBuffer(emptyPatchProto.Data()), emptyPatchProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    TStateHeader header;
    EXPECT_TRUE(ParseHeader(result.Data + result.Offset, header));
    EXPECT_EQ(TStateHeader::NONE_TYPE, header.type());
    EXPECT_TRUE(header.has_error_code());
    EXPECT_EQ(TStateHeader::MERGE_PATCHES_ERROR, header.error_code());

    Free(result);
}

TEST(XDeltaMergeProto, MergeEmptyPatchNonEmptyPatch)
{
    auto base = "";
    auto baseSize = 0;
    auto state = "123456";
    auto stateSize = strlen(state);
    auto patchProto = EncodePatchProtoAsBuffer(ByteBuffer(base), baseSize, ByteBuffer(state), stateSize);
    auto hash = CalcHash(ByteBuffer(base), baseSize);
    auto emptyPatchProto = EncodePatchProtoAsBuffer(
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        [hash](auto& hdr) { hdr.set_base_hash(hash); hdr.set_state_hash(hash); });

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(emptyPatchProto.Data()), emptyPatchProto.Size(), ByteBuffer(patchProto.Data()), patchProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    TStateHeader header;
    EXPECT_TRUE(ParseHeader(result.Data + result.Offset, header));
    EXPECT_EQ(TStateHeader::PATCH, header.type());

    TStateHeader patch1;
    ParseHeader(ByteBuffer(patchProto.Data()), patch1);
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(patch1, header));
    EXPECT_TRUE(header.data_size());
    EXPECT_EQ(0, memcmp(result.Data + result.Offset + SizeOfHeader(header), patchProto.Data() + SizeOfHeader(patch1), header.data_size()));

    Free(result);
}

TEST(XDeltaMergeProto, MergePatches)
{
    auto base = "12345";
    auto baseSize = strlen(base);
    auto state1 = "12243465";
    auto state1Size = strlen(state1);
    auto state2 = "125673";
    auto state2Size = strlen(state2);

    auto patch1Proto = EncodePatchProtoAsBuffer(ByteBuffer(base), baseSize, ByteBuffer(state1), state1Size);
    TStateHeader patch1;
    EXPECT_TRUE(ParseHeader(ByteBuffer(patch1Proto.Data()), patch1));

    auto patch2Proto = EncodePatchProtoAsBuffer(ByteBuffer(state1), state1Size, ByteBuffer(state2), state2Size);
    TStateHeader patch2;
    EXPECT_TRUE(ParseHeader(ByteBuffer(patch2Proto.Data()), patch2));

    TSpan result;
    auto done = MergeStates(nullptr, ByteBuffer(patch1Proto.Data()), patch1Proto.Size(), ByteBuffer(patch2Proto.Data()), patch2Proto.Size(), &result);
    EXPECT_TRUE(done);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    TStateHeader merged;
    EXPECT_TRUE(ParseHeader(result.Data + result.Offset, merged));
    EXPECT_EQ(TStateHeader::PATCH, merged.type());

    auto payloadData = result.Data + result.Offset + SizeOfHeader(merged);
    auto payloadSize = result.Size - SizeOfHeader(merged);
    size_t stateSize = 0;
    auto state = TDataPtr(ApplyPatch(nullptr, 0, ByteBuffer(base), baseSize, payloadData, payloadSize, merged.state_size(), &stateSize));
    EXPECT_EQ(stateSize, state2Size);
    EXPECT_TRUE(state.get());
    EXPECT_EQ(0, memcmp(state.get(), state2, stateSize));

    EXPECT_EQ(patch1.base_hash(), merged.base_hash());
    EXPECT_EQ(patch2.state_hash(), merged.state_hash());
    EXPECT_EQ(patch2.state_size(), merged.state_size());

    Free(result);
}

TEST(XDeltaMergeProto, ApplyPatchWrongStateHash)
{
    auto base = "12345";
    auto baseSize = strlen(base);
    auto state = "12243465";
    auto stateSize = strlen(state);

    size_t patchSize = 0;
    auto patch = TDataPtr(ComputePatch(nullptr, ByteBuffer(base), baseSize, ByteBuffer(state), stateSize, &patchSize));

    auto baseProto = EncodeBaseProtoAsBuffer(ByteBuffer(base), baseSize);
    auto patchProto = EncodePatchProtoAsBuffer(
        ByteBuffer(base),
        baseSize,
        ByteBuffer(state),
        stateSize,
        patch.get(),
        patchSize,
        [](auto& hdr) { hdr.set_state_hash(hdr.state_hash() + 1); });

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(baseProto.Data()), baseProto.Size(), ByteBuffer(patchProto.Data()), patchProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    TStateHeader header;
    EXPECT_TRUE(ParseHeader(result.Data + result.Offset, header));
    EXPECT_EQ(TStateHeader::NONE_TYPE, header.type());
    EXPECT_EQ(TStateHeader::STATE_HASH_ERROR, header.error_code());

    Free(result);
}

TEST(XDeltaMergeProto, ApplyPatchWrongBaseHash)
{
    auto base = "12345";
    auto baseSize = strlen(base);
    auto state = "12243465";
    auto stateSize = strlen(state);

    auto baseProto = EncodeBaseProtoAsBuffer(ByteBuffer(base), baseSize);
    auto patchProto = EncodePatchProtoAsBuffer(
        ByteBuffer(base),
        baseSize,
        ByteBuffer(state),
        stateSize,
        [](auto& hdr) { hdr.set_base_hash(hdr.base_hash() + 1); });

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(baseProto.Data()), baseProto.Size(), ByteBuffer(patchProto.Data()), patchProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    TStateHeader header;
    EXPECT_TRUE(ParseHeader(result.Data + result.Offset, header));
    EXPECT_EQ(TStateHeader::NONE_TYPE, header.type());
    EXPECT_EQ(TStateHeader::BASE_HASH_ERROR, header.error_code());

    Free(result);
}

TEST(XDeltaMergeProto, OverwriteStateMergeBaseBase)
{
    auto base = "12345";
    auto baseSize = strlen(base);
    auto state = "12243465";
    auto stateSize = strlen(state);

    auto baseProto = EncodeBaseProtoAsBuffer(ByteBuffer(base), baseSize);
    auto stateProto = EncodeBaseProtoAsBuffer(ByteBuffer(state), stateSize);

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(baseProto.Data()), baseProto.Size(), ByteBuffer(stateProto.Data()), stateProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);
    EXPECT_EQ(result.Size, stateProto.Size());
    EXPECT_EQ(0, memcmp(result.Data, stateProto.Data(), stateProto.Size()));

    if (result.Data != reinterpret_cast<const ui8*>(stateProto.Data())) {
        Free(result);
    }
}

TEST(XDeltaMergeProto, OverwriteStateWithNewBase)
{
    auto base = "12345";
    auto baseSize = strlen(base);
    auto state = "12243465";
    auto stateSize = strlen(state);

    auto baseProto = EncodeBaseProtoAsBuffer(ByteBuffer(base), baseSize);
    auto patchProto = EncodePatchProtoAsBuffer(ByteBuffer(base), baseSize, ByteBuffer(state), stateSize);

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(patchProto.Data()), patchProto.Size(), ByteBuffer(baseProto.Data()), baseProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);
    EXPECT_EQ(result.Size, baseProto.Size());
    EXPECT_EQ(0, memcmp(result.Data, baseProto.Data(), baseProto.Size()));

    if (result.Data != reinterpret_cast<const ui8*>(baseProto.Data())) {
        Free(result);
    }
}

TEST(XDeltaMergeProto, MergeLeftErrorRightPatch) {
    auto base = "12345";
    auto baseSize = strlen(base);
    auto state = "12243465";
    auto stateSize = strlen(state);

    auto errorProto = EncodeErrorProtoAsBuffer(TStateHeader::BASE_HASH_ERROR);
    auto patchProto = EncodePatchProtoAsBuffer(ByteBuffer(base), baseSize, ByteBuffer(state), stateSize);

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(errorProto.Data()), errorProto.Size(), ByteBuffer(patchProto.Data()), patchProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    NProtoBuf::Arena arena;
    TState error(arena, result.Data + result.Offset, result.Size);
    EXPECT_EQ(TStateHeader::NONE_TYPE, error.Type());
    EXPECT_EQ(TStateHeader::BASE_HASH_ERROR, error.Error());
    EXPECT_FALSE(error.PayloadSize());
    EXPECT_FALSE(error.PayloadData());

    Free(result);
}

TEST(XDeltaMergeProto, MergeLeftErrorRightBase) {
    auto base = "12345";
    auto baseSize = strlen(base);

    auto errorProto = EncodeErrorProtoAsBuffer(TStateHeader::BASE_HASH_ERROR);
    auto baseProto = EncodeBaseProtoAsBuffer(ByteBuffer(base), baseSize);

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(errorProto.Data()), errorProto.Size(), ByteBuffer(baseProto.Data()), baseProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);
    EXPECT_EQ(result.Size, baseProto.Size());
    EXPECT_EQ(0, memcmp(result.Data + result.Offset, baseProto.Data(), baseProto.Size()));

    if (result.Data != reinterpret_cast<const ui8*>(baseProto.Data())) {
        Free(result);
    }
}

TEST(XDeltaMergeProto, MergeLeftBaseRightError) {
    auto base = "12345";
    auto baseSize = strlen(base);

    auto errorProto = EncodeErrorProtoAsBuffer(TStateHeader::STATE_HASH_ERROR);
    auto baseProto = EncodeBaseProtoAsBuffer(ByteBuffer(base), baseSize);

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(baseProto.Data()), baseProto.Size(), ByteBuffer(errorProto.Data()), errorProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    NProtoBuf::Arena arena;
    TState error(arena, result.Data + result.Offset, result.Size);
    EXPECT_EQ(TStateHeader::NONE_TYPE, error.Type());
    EXPECT_EQ(TStateHeader::STATE_HASH_ERROR, error.Error());

    Free(result);
}

TEST(XDeltaMergeProto, MergeLeftPatchRightError)
{
    auto base = "12345";
    auto baseSize = strlen(base);
    auto state = "12243465";
    auto stateSize = strlen(state);

    auto errorProto = EncodeErrorProtoAsBuffer(TStateHeader::BASE_HASH_ERROR);
    auto patchProto = EncodePatchProtoAsBuffer(ByteBuffer(base), baseSize, ByteBuffer(state), stateSize);

    TSpan result;
    auto merged = MergeStates(nullptr, ByteBuffer(patchProto.Data()), patchProto.Size(), ByteBuffer(errorProto.Data()), errorProto.Size(), &result);
    EXPECT_TRUE(merged);
    EXPECT_TRUE(result.Data);
    EXPECT_TRUE(result.Size);

    NProtoBuf::Arena arena;
    TState error(arena, result.Data + result.Offset, result.Size);
    EXPECT_EQ(TStateHeader::NONE_TYPE, error.Type());
    EXPECT_EQ(TStateHeader::BASE_HASH_ERROR, error.Error());
    EXPECT_FALSE(error.PayloadSize());
    EXPECT_FALSE(error.PayloadData());

    Free(result);
}
