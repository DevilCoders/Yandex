#include "codecs.h"

#include <util/stream/zlib.h>
#include <util/stream/str.h>
#include <util/stream/mem.h>

#include <library/cpp/comptable/comptable.h>
#include <util/random/random.h>

namespace NOmni {
    IStatCollector::IStatCollector()
        : Done(0)
    {
    }

    void IStatCollector::AddStat(TStringBuf data) {
        Y_ASSERT(!Done);
        DoAddStat(data);
    }

    TBlob IStatCollector::BuildTable() {
        Done = 1;
        return DoBuildTable();
    }

    class TNoneCollector: public IStatCollector {
    public:
        TBlob DoBuildTable() override {
            return TBlob();
        }
        void DoAddStat(TStringBuf data) override {
            Y_UNUSED(data);
        }
    };

    class TComptableCollector: public IStatCollector {
    public:
        TBlob DoBuildTable() override {
            NCompTable::TCompressorTable tbl;
            Sampler.BuildTable(tbl);
            return TBlob::Copy(&tbl, sizeof(tbl));
        }
        void DoAddStat(TStringBuf data) override {
            Sampler.AddStat(data);
        }

    private:
        NCompTable::TDataSampler Sampler;
    };

    class TNoneCompressor: public ICompressor {
    public:
        TNoneCompressor(TBlob rawTable) {
            Y_UNUSED(rawTable);
        }
        void Compress(TStringBuf data, TVector<char>* dst) override {
            dst->yresize(data.size());
            memcpy(dst->data(), data.data(), data.size());
        }
    };

    template <bool HQ>
    class TComptableCompressor: public ICompressor {
    public:
        TComptableCompressor(TBlob rawTable) {
            Y_ASSERT(sizeof(NCompTable::TCompressorTable) == rawTable.Size());
            NCompTable::TCompressorTable table = *reinterpret_cast<const NCompTable::TCompressorTable*>(rawTable.Data());
            Compressor.Reset(new NCompTable::TChunkCompressor(HQ, table));
        }
        void Compress(TStringBuf data, TVector<char>* dst) override {
            Compressor->Compress(data, dst);
        }

    private:
        THolder<NCompTable::TChunkCompressor> Compressor;
    };

    class TZLibCompressor: public ICompressor {
    public:
        TZLibCompressor(TBlob rawTable) {
            Y_UNUSED(rawTable);
        }
        void Compress(TStringBuf data, TVector<char>* dst) override {
            TStringStream buf;
            TZLibCompress compressor(&buf, ZLib::ZLib);
            compressor.Write(data.data(), data.size());
            compressor.Finish();
            dst->resize(buf.Size());
            memcpy(dst->data(), buf.Data(), buf.Size());
        }
    };

    class TNoneDecompressor: public IDecompressor {
    public:
        TNoneDecompressor(TBlob rawTable) {
            Y_UNUSED(rawTable);
        }
        void Decompress(TStringBuf data, TVector<char>* dst) override {
            dst->yresize(data.size());
            memcpy(dst->data(), data.data(), data.size());
        }
    };

    template <bool HQ>
    class TComptableDecompressor: public IDecompressor {
    public:
        TComptableDecompressor(TBlob rawTable) {
            Y_ASSERT(sizeof(NCompTable::TCompressorTable) == rawTable.Size());
            NCompTable::TCompressorTable table = *reinterpret_cast<const NCompTable::TCompressorTable*>(rawTable.Data());
            Decompressor.Reset(new NCompTable::TChunkDecompressor(HQ, table));
        }
        void Decompress(TStringBuf data, TVector<char>* dst) override {
            Decompressor->Decompress(data, dst);
        }

    private:
        THolder<NCompTable::TChunkDecompressor> Decompressor;
    };

    class TZLibDecompressor: public IDecompressor {
    public:
        TZLibDecompressor(TBlob rawTable) {
            Y_UNUSED(rawTable);
        }
        void Decompress(TStringBuf data, TVector<char>* dst) override {
            TMemoryInput in(data.data(), data.size());
            TZLibDecompress decompressor(&in);
            dst->clear();
            size_t chunkSize = data.size() + 1;
            size_t p = 0;
            while (1) {
                dst->resize(dst->size() + chunkSize);
                size_t nread = decompressor.Load(dst->data() + p, chunkSize);
                p += nread;
                Y_ASSERT(nread <= chunkSize);
                if (nread < chunkSize)
                    break;
                chunkSize = 1.5 * chunkSize + 1;
            }
            dst->resize(p);
        }
    };

    struct ICodecSuite {
        virtual IStatCollector* DoCreateCollector() const = 0;
        virtual ICompressor* DoCreateCompressor(TBlob table) const = 0;
        virtual IDecompressor* DoCreateDecompressor(TBlob table) const = 0;
        virtual ~ICodecSuite() {
        }
    };

    template <typename TCollector, typename TCompressor, typename TDecompressor>
    struct TCodecSuite: public ICodecSuite {
        IStatCollector* DoCreateCollector() const override {
            return new TCollector();
        }
        ICompressor* DoCreateCompressor(TBlob table) const override {
            return new TCompressor(table);
        }
        IDecompressor* DoCreateDecompressor(TBlob table) const override {
            return new TDecompressor(table);
        }
    };

    struct TCodecManager {
        TCodecManager()
            : CodecSuites(CT_MAX)
        {
            CodecSuites[CT_NONE] = new TCodecSuite<TNoneCollector, TNoneCompressor, TNoneDecompressor>();
            CodecSuites[CT_COMPTABLE] = new TCodecSuite<TComptableCollector, TComptableCompressor<false>, TComptableDecompressor<false>>();
            CodecSuites[CT_COMPTABLE_HQ] = new TCodecSuite<TComptableCollector, TComptableCompressor<true>, TComptableDecompressor<true>>();
            CodecSuites[CT_ZLIB] = new TCodecSuite<TNoneCollector, TZLibCompressor, TZLibDecompressor>();
        }
        TVector<TAtomicSharedPtr<ICodecSuite>> CodecSuites;

    public:
        static const TCodecManager& SingletonManager() {
            static TCodecManager codecManager;
            ;
            return codecManager;
        }
    };

    IStatCollector* CreateCollectorByType(ECodecType codecType) {
        return TCodecManager::SingletonManager().CodecSuites[codecType]->DoCreateCollector();
    }
    ICompressor* CreateCompressorByType(ECodecType codecType, TBlob table) {
        return TCodecManager::SingletonManager().CodecSuites[codecType]->DoCreateCompressor(table);
    }
    IDecompressor* CreateDecompressorByType(ECodecType codecType, TBlob table) {
        return TCodecManager::SingletonManager().CodecSuites[codecType]->DoCreateDecompressor(table);
    }

}
