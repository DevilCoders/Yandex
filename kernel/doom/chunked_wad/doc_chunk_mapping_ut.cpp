#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/str.h>

#include <kernel/doom/chunked_wad/protos/doc_chunk_mapping_union.pb.h>
#include <kernel/doom/chunked_wad/doc_chunk_mapping_searcher.h>
#include <kernel/doom/wad/mega_wad_writer.h>
#include <kernel/doom/wad/wad.h>

using namespace NDoom;

namespace {
    THolder<NDoom::IWad> WadFromProtoMapping(const google::protobuf::Message& m) {
        TString serializedMapping;
        Y_ENSURE(m.SerializeToString(&serializedMapping));

        TString string;
        TStringOutput outStream(string);
        NDoom::TMegaWadWriter writer(&outStream);
        *writer.StartGlobalLump(TWadLumpId(ChunkMappingIndexType, EWadLumpRole::Struct)) << serializedMapping;
        writer.Finish();

        return IWad::Open(TBlob::FromString(std::move(string)));
    }
}

Y_UNIT_TEST_SUITE(DocChunkMapping) {
    Y_UNIT_TEST(ModuloMappingOld) {
        TModuloDocChunkMappingInfo info;
        info.SetModulo(16);
        info.SetSize(100);

        THolder<IWad> wad = WadFromProtoMapping(info);
        TDocChunkMappingSearcher searcher(wad.Get());
        UNIT_ASSERT_EQUAL(searcher.Find(0), TDocChunkMapping(0, 0));
        UNIT_ASSERT_EQUAL(searcher.Find(15), TDocChunkMapping(15, 0));
        UNIT_ASSERT_EQUAL(searcher.Find(16), TDocChunkMapping(0, 1));
    }

    Y_UNIT_TEST(ModuloMappingNewViaUnion) {
        TModuloDocChunkMappingInfo info;
        info.SetModulo(16);
        info.SetSize(100);
        TDocChunkMappingUnion un;
        *un.MutableModuloMapping() = info;

        THolder<IWad> wad = WadFromProtoMapping(un);
        TDocChunkMappingSearcher searcher(wad.Get());
        UNIT_ASSERT_EQUAL(searcher.Find(0), TDocChunkMapping(0, 0));
        UNIT_ASSERT_EQUAL(searcher.Find(15), TDocChunkMapping(15, 0));
        UNIT_ASSERT_EQUAL(searcher.Find(16), TDocChunkMapping(0, 1));
    }

    Y_UNIT_TEST(SequentialMapping) {
        TSequentialDocChunkMappingInfo info;
        info.AddIndices(0);
        info.AddIndices(5);
        info.AddIndices(8);
        info.AddIndices(100);
        TDocChunkMappingUnion un;
        *un.MutableSequentialMapping() = info;

        THolder<IWad> wad = WadFromProtoMapping(un);
        TDocChunkMappingSearcher searcher(wad.Get());
        UNIT_ASSERT_EQUAL(searcher.Find(0), TDocChunkMapping(0, 0));
        UNIT_ASSERT_EQUAL(searcher.Find(1), TDocChunkMapping(0, 1));
        UNIT_ASSERT_EQUAL(searcher.Find(5), TDocChunkMapping(1, 0));
        UNIT_ASSERT_EQUAL(searcher.Find(7), TDocChunkMapping(1, 2));
        UNIT_ASSERT_EQUAL(searcher.Find(99), TDocChunkMapping(2, 91));
        UNIT_ASSERT_EQUAL(searcher.Find(100), TDocChunkMapping::Invalid);
    }
}
