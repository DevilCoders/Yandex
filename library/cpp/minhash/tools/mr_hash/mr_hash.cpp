#include "mr_hash.h"

#include <library/cpp/minhash/minhash_func.h>
#include <library/cpp/minhash/minhash_builder.h>
#include <library/cpp/minhash/minhash_helpers.h>
#include <library/cpp/minhash/table.h>

#include <mapreduce/lib/all.h>
#include <mapreduce/library/temptable/temptable.h>
#include <mapreduce/library/seq_calc/seq_calc.h>

#include <util/datetime/cputimer.h>
#include <util/string/printf.h>
#include <util/string/vector.h>
#include <util/stream/mem.h>
#include <util/stream/buffer.h>
#include <library/cpp/streams/factory/factory.h>
#include <util/stream/file.h>
#include <util/generic/algorithm.h>
#include <util/folder/dirut.h>
#include <util/system/yassert.h>
#include <util/string/split.h>

using namespace NMR;

namespace NMinHash {
    const ui64 MAX_PORTION_SIZE = 1 << 28;
    const ui8 USUAL_KEY_SIZE = 3;

    class TPortionsMap: public NMR::IMap {
        ui32 NumPortions_;

    public:
        TPortionsMap()
            : NumPortions_()
        {
        }

        TPortionsMap(ui32 numPortions)
            : NumPortions_(numPortions)
        {
        }

        int operator&(IBinSaver& s)override {
            s.Add(0, &NumPortions_);
            return 0;
        }

        void Do(NMR::TValue key, NMR::TValue val, NMR::TUpdate& output) const {
            DoImpl(key, val, output);
        }

        void Do(NMR::TValue key, NMR::TValue val, NMR::TUpdate& output) override {
            DoImpl(key, val, output);
        }

        void DoImpl(NMR::TValue key, NMR::TValue val, NMR::TUpdate& output) const {
            output.AddSub(Sprintf("%08" PRIx32, TDistChdMinHashFunc::Bucket(key.GetData(), key.GetSize(), NumPortions_)), key, val);
        }

        OBJECT_METHODS(TPortionsMap)
    };

    class TBuildHashReduce: public NMR::IReduce {
        double LoadFactor_;
        ui32 KeysPerBucket_;
        ui8 FprSize_;
        bool Wide_;

    public:
        TBuildHashReduce()
            : LoadFactor_()
            , KeysPerBucket_()
            , FprSize_()
            , Wide_()
        {
        }

        TBuildHashReduce(double loadFactor, ui32 keysPerBucket, ui8 fprSize, bool wide = false)
            : LoadFactor_(loadFactor)
            , KeysPerBucket_(keysPerBucket)
            , FprSize_(fprSize)
            , Wide_(wide)
        {
        }

        int operator&(IBinSaver& s)override {
            s.Add(0, &LoadFactor_);
            s.Add(1, &KeysPerBucket_);
            s.Add(2, &FprSize_);
            s.Add(3, &Wide_);
            return 0;
        }

        void Do(NMR::TValue k, NMR::TTableIterator& it, NMR::TUpdate& output) const {
            DoImpl(k, it, output);
        }

        void Do(NMR::TValue k, NMR::TTableIterator& it, NMR::TUpdate& output) override {
            DoImpl(k, it, output);
        }

        void DoImpl(NMR::TValue k, NMR::TTableIterator& it, NMR::TUpdate& output) const {
            TVector<TString> keys;
            TVector<TString> values;
            TUtf16String w;
            TString s;
            for (; it.IsValid(); ++it) {
                if (Wide_) {
                    w.AssignUtf8(it.GetSubKey().AsStringBuf());
                    s.assign((const char*)w.Data(), w.Size() * sizeof(TUtf16String::TChar));
                } else {
                    s.assign(it.GetSubKey().GetData(), it.GetSubKey().GetSize());
                }
                values.push_back(it.GetValue().AsString());
                keys.push_back(s);
            }

            size_t numKeys = keys.size();
            TVector<TString>::iterator i1 = Unique(keys.begin(), keys.end());
            size_t newSize = i1 - keys.begin();
            if (newSize < numKeys) {
                ythrow yexception() << "non unique keys for portion" << k.AsStringBuf();
            }

            TChdHashBuilder builder(keys.size(), LoadFactor_, KeysPerBucket_, 0, FprSize_);
            TAutoPtr<TChdMinHashFunc> hash;

            do {
                try {
                    hash = builder.Build(keys);
                    break;
                } catch (THashBuildException&) {
                    Cerr << "failed to build portion " << k.AsString() << Endl;
                    continue;
                }
            } while (1);
            output.SetCurrentTable(0);
            TBufferOutput out;
            hash->SaveLoad(&out);
            output.AddSub(k, Sprintf("0x%08" PRIx32, hash->Size()), TValue(out.Buffer().Data(), out.Buffer().Size()));

            output.SetCurrentTable(1);
            for (size_t i2 = 0; i2 < keys.size(); ++i2) {
                ui32 pos = hash->Get(keys[i2].data(), keys[i2].size(), false);
                if (pos == TChdMinHashFunc::npos) {
                    ythrow yexception() << "invalid hash for " << keys[i2];
                }
                output.AddSub(k, Sprintf("0x%08" PRIx32, pos), values[i2]);
            }
        }

        OBJECT_METHODS(TBuildHashReduce);
    };

    class THashValuesMap: public NMR::IMap {
        TString FileName_;
        THolder<TDistChdMinHashFunc> Hash_;

    public:
        THashValuesMap() {
        }

        THashValuesMap(const TString fileName)
            : FileName_(fileName)
        {
        }

        int operator&(IBinSaver& s)override {
            s.Add(0, &FileName_);
            return 0;
        }

        void Start(ui32 jobId, ui64 recordStart, TUpdate& output) override {
            Y_UNUSED(jobId);
            Y_UNUSED(recordStart);
            Y_UNUSED(output);
            Hash_.Reset(new TDistChdMinHashFunc(FileName_));
        }

        void Do(NMR::TValue key, NMR::TValue val, NMR::TUpdate& output) override {
            TDistChdMinHashFunc::TDomain h = Hash_->Get(key.GetData(), key.GetSize());
            output.AddSub(Sprintf("0x%08l" PRIx32, h), key, val);
        }

        OBJECT_METHODS(THashValuesMap)
    };

    ui32 SafeNumPortions(TServer& server, const TString& tableName, ui32 numPortions, double loadFactor, ui32 keysPerBucket, ui8 fprSize) {
        Y_UNUSED(loadFactor);
        TClient client(server);
        NMR::TTable table(client, tableName);
        ui64 tableSize = table.GetSize();
        ui64 numKeys = table.GetRecordCount();
        ui64 keySize = tableSize / numKeys;
        ui32 numBuckets = numKeys / keysPerBucket + 1;
        ui64 hashKeySize = 1.0 / numKeys * (numKeys * 12 +                    //Itesm_
                                                    numBuckets * 12 +         //Buckest_
                                                    numKeys * 16 +            //MapItems_
                                                    numBuckets * sizeof(32) + //Displ_
                                                    (numBuckets / 64) +       //FreeBins_
                                                    fprSize
                                                ? numKeys * sizeof(32)
                                                : 0); //Fpr_

        ui64 size = keySize + hashKeySize;
        ui64 n = 1;
        ui64 maxmem = 3 * static_cast<ui32>(1) << 30;
        while (n * size < maxmem) {
            n *= 2;
        }
        if (!numPortions) {
            numPortions = numKeys / n;
        }
        ui64 hashSize = (USUAL_KEY_SIZE + fprSize) * numKeys;
        ui64 portionSize = hashSize / numPortions;
        if (portionSize > MAX_PORTION_SIZE) {
            numPortions = hashSize / MAX_PORTION_SIZE;
        }
        return numPortions;
    }

    void CreateTable(const TString& serverName, const TString& tableName, const TString& fileName) {
        TServer server(serverName);
        TClient client(server);

        /*WithUniqBTTable values(server, "mr_hash/" + tableName + "/values", true);
    {
        TTimeLogger lg("Mapping values...");
        TMRParams p(tableName, values.Name());
        p.AddFile(fileName);
        //server.Map(p, new THashValuesMap(GetFileNameComponent(~fileName)));
        lg.SetOK();
    }

    {
        TTimeLogger lg("Sorting values...");
        //server.Sort(values.Name(), values.Name());
        lg.SetOK();
    }

    {
        TTimeLogger lg("Building values...");
        NMR::TTable t(client, tableName);
        TVector<TString> parts;
        ui64 numRecords = t.GetRecordCount();
        TTableIterator it = t.Begin();
        size_t numColumns = Count(it.GetValue().Begin(), it.GetValue().End(), '\t') + 1;
        TVector<TVector<ui64> > cols(numColumns);
        for (size_t i = 0; i < cols.size(); ++i) {
            cols[i].resize(numRecords, 0);
        }
        TAutoPtr<TDistChdMinHashFunc> hash(new TDistChdMinHashFunc(fileName));
        for (TTableIterator it = t.Begin(); it.IsValid(); ++it) {
            StringSplitter(it.GetValue().AsString()).Split('\t').SkipEmpty().Collect(&parts);
            parts.resize(numColumns, 0);
            ui64 pos = hash->Get(it.GetKey().GetData(), it.GetKey().GetSize());
            for (size_t j = 0; j < cols.size(); ++j) {
                cols[j][pos] = FromString<ui64>(parts[j]);
            }
        }
        NMinHash::TTable<ui64, TDistChdMinHashFunc> tbl(hash);
        for (size_t i = 0; i < cols.size(); ++i) {
            tbl.Add(cols[i].begin(), cols[i].end());
        }
        TAutoPtr<IOutputStream> out = OpenOutput(fileName);
        ::Save(out.Get(), tbl);
        lg.SetOK();
    }*/

        if (0) {
            TTimeLogger lg("Verifying...");
            NMR::TTable t(client, tableName);
            TVector<TString> parts;
            TTableIterator it1 = t.Begin();
            size_t numColumns = Count(it1.GetValue().Begin(), it1.GetValue().End(), '\t') + 1;

            NMinHash::TTable<ui64, TDistChdMinHashFunc> tbl(fileName);

            for (TTableIterator it2 = t.Begin(); it2.IsValid(); ++it2) {
                StringSplitter(it2.GetValue().AsString()).Split('\t').SkipEmpty().Collect(&parts);
                parts.resize(numColumns);
                ui64 pos = tbl.Hash()->Get(it2.GetKey().GetData(), it2.GetKey().GetSize());
                if (pos == static_cast<ui64>(-1)) {
                    Cerr << "key not found: " << it2.GetKey().AsStringBuf() << Endl;
                    continue;
                }
                for (size_t j = 0; j < parts.size(); ++j) {
                    if (tbl.Get(j, pos) != FromString<ui64>(parts[j])) {
                        Cerr << "invalid value " << tbl.Get(j, pos)
                             << " for key " << it2.GetKey().AsStringBuf()
                             << ", expected " << FromString<ui64>(parts[j]) << Endl;
                        continue;
                    }
                }
            }
            lg.SetOK();
        }
    }

    void CreateHash(const TString& serverName, const TString& tableName, ui32 numPortions, double loadFactor, ui32 keysPerBucket, ui8 fprSize, bool wide, const TString& fileName) {
        TServer server(serverName);
        TClient client(server);
        numPortions = SafeNumPortions(server, tableName, numPortions, loadFactor, keysPerBucket, fprSize);

        WithUniqBTTable portions(server, "mr_hash/" + tableName + "/portions", true);
        {
            TTimeLogger lg(Sprintf("Splitting dataset into %d portions", numPortions));
            TMRParams p(tableName, portions.Name());
            server.Map(p, new TPortionsMap(numPortions));
            lg.SetOK();
        }

        {
            TTimeLogger lg("Sorting...");
            server.Sort(portions.Name(), portions.Name());
            lg.SetOK();
        }

        WithUniqBTTable hashes(server, "mr_hash/" + tableName + "/portions", true);
        WithUniqBTTable values(server, "mr_hash/" + tableName + "/values", true);
        {
            TTimeLogger lg("Building hashes");
            TMRParams p(portions.Name(), hashes.Name());
            p.AddOutputTable(values.Name());
            p.SetCPUIntensiveMode();
            p.SetThreadCount(8);
            p.SetJobCount(numPortions);
            server.Reduce(p, new TBuildHashReduce(loadFactor, keysPerBucket, fprSize, wide));
            lg.SetOK();
        }

        {
            TTimeLogger lg("Sorting...");
            server.Sort(hashes.Name(), hashes.Name());
            server.Sort(values.Name(), values.Name());
            lg.SetOK();
        }

        {
            TTimeLogger lg("Saving...");
            TAutoPtr<IOutputStream> out = OpenOutput(fileName);
            TDistChdHashBuilder dht(numPortions, loadFactor, keysPerBucket, fprSize, out.Get());
            NMR::TTable t(client, hashes.Name());
            for (TTableIterator it = t.Begin(); it.IsValid(); ++it) {
                TValue val = it.GetValue();
                TMemoryInput mi(val.GetData(), val.GetSize());
                TChdMinHashFunc hash(&mi);
                dht.Add(hash);
            }
            dht.Finish();
            lg.SetOK();
        }

        {
            TTimeLogger lg("Verifying...");
            TDistChdMinHashFunc h(fileName);
            lg.SetOK();
        }

        {
            TTimeLogger lg("Building values...");
            NMR::TTable t(client, values.Name());
            TVector<TString> parts;
            ui64 numRecords = t.GetRecordCount();
            TTableIterator it1 = t.Begin();
            size_t numColumns = Count(it1.GetValue().Begin(), it1.GetValue().End(), '\t') + 1;
            TVector<TVector<ui64>> cols(numColumns);
            for (size_t i = 0; i < cols.size(); ++i) {
                cols[i].resize(numRecords, 0);
            }

            ui64 pos = 0;
            for (TTableIterator it2 = t.Begin(); it2.IsValid(); ++it2, ++pos) {
                StringSplitter(it2.GetValue().AsString()).Split('\t').SkipEmpty().Collect(&parts);
                parts.resize(numColumns, nullptr);
                for (size_t j = 0; j < cols.size(); ++j) {
                    cols[j][pos] = FromString<ui64>(parts[j]);
                }
            }

            NMinHash::TTable<ui64, TDistChdMinHashFunc> tbl(new TDistChdMinHashFunc(fileName));
            for (size_t i = 0; i < cols.size(); ++i) {
                tbl.Add(cols[i].begin(), cols[i].end());
            }
            TAutoPtr<IOutputStream> out = OpenOutput(fileName);
            ::Save(out.Get(), tbl);
            lg.SetOK();
        }
    }

}

using namespace NMinHash;
REGISTER_SAVELOAD_CLASS(0x1, TPortionsMap);
REGISTER_SAVELOAD_CLASS(0x2, TBuildHashReduce);
REGISTER_SAVELOAD_CLASS(0x3, THashValuesMap);
