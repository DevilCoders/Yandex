#include "compressor.h"

#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/arcface.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/array_ref.h>

Y_UNIT_TEST_SUITE(ExtArc) {


    Y_UNIT_TEST(Sample) {
        TString sampleBlob;
        {
            TDocDescr descr;
            TDocInfoExtWriter writer;
            TBuffer newInfo;
            descr.UseBlob(&newInfo);
            descr.SetUrl("url");
            descr.GetMutableUrlDescr().HostId = 25;

            THashMap<TString, TString> infos;
            infos["key1"] = "aaaaaaaaaaa";
            infos["key2"] = "aaabbbbbbbbbb";
            infos["key3"] = "aaabbbbbbbbbb";
            infos["key3.1"] = "abbabah";
            infos["key3.2"] = "abbabcc";
            infos["key4"] = "ccccccccccccc";
            infos["key5"] = "dc";
            for (auto kv : infos) {
                writer.Add(kv.first.data(), kv.second.data());
            }
            writer.Write(descr);

            TStringOutput out(sampleBlob);
            WriteEmptyDoc(out, newInfo.data(), descr.CalculateBlobSize(), nullptr, 0, 0);
            out.Finish();
            sampleBlob = sampleBlob.substr(sizeof(TArchiveHeader));
        }
        TVector<TString> keys = ExtractExtArcKeys(TBlob::NoCopy(sampleBlob.data(), sampleBlob.size()));
        TVector<NJupiter::TExtArcKey> extkeys;
        for (auto key : keys) {
            extkeys.emplace_back();
            extkeys.back().SetKey(key);
            extkeys.back().SetId(extkeys.size() - 1);
        }
        TBlob compressedBlob = CompressExtArc(TBlob::NoCopy(sampleBlob.data(), sampleBlob.size()), extkeys);
        TBlob decompressedBlob = DecompressExtArc(compressedBlob, keys);
        //auto decompressedBlob = sampleBlob;
        {
            TDocDescr descr;
            THashMap<TString, TString> docInfo;
            descr.UseBlob(decompressedBlob.AsCharPtr(), decompressedBlob.Size());
            descr.ConfigureDocInfos(docInfo);
            UNIT_ASSERT_STRINGS_EQUAL(descr.get_url(), "url");
            UNIT_ASSERT_EQUAL(descr.get_hostid(), 25);
            UNIT_ASSERT_STRINGS_EQUAL(docInfo["key1"], "aaaaaaaaaaa");
            UNIT_ASSERT_STRINGS_EQUAL(docInfo["key2"], "aaabbbbbbbbbb");
            UNIT_ASSERT_STRINGS_EQUAL(docInfo["key3"], "aaabbbbbbbbbb");
            UNIT_ASSERT_STRINGS_EQUAL(docInfo["key3.1"], "abbabah");
            UNIT_ASSERT_STRINGS_EQUAL(docInfo["key3.2"], "abbabcc");
            UNIT_ASSERT_STRINGS_EQUAL(docInfo["key4"], "ccccccccccccc");
            UNIT_ASSERT_STRINGS_EQUAL(docInfo["key5"], "dc");

        }

    }
}
