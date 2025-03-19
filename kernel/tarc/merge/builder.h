#pragma once

#include <kernel/tarc/iface/arcface.h>

#include <library/cpp/object_factory/object_factory.h>

#include <kernel/tarc/dirconf/dirconf.h>
#include <kernel/tarc/enums/searcharc.h>
#include <kernel/multipart_archive/config/config.h>

#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/string/cast.h>

class ITextArchiveBuilder {
public:
    static ui32 DeletedDocument() {
        return Max<ui32>();
    }

    struct TConstructContext {
        TDirConf DirConfig;
        ui32 ThreadsCount = 1;
        ui32 DocsCount = 0;
        TArchiveVersion ArchiveVersion = ARCVERSION;
        NRTYArchive::TMultipartConfig MultipartConfig;
    };

    void IndexDoc(ui32 docId, const TBlob& archive, ui32 thread) {
        Archives[thread]->Write(docId, archive);
    }

    void Start() {
        for (const auto& archive : Archives) {
            archive->WriteTextArchiveHeader();
        }
    }

    void Stop() {
        for (const auto& archive : Archives) {
            archive->Finish();
        }
    }

    void Close(const TVector<ui32>* remapTable) {
        TMergerContext ctx(remapTable);
        ctx.OutputPath  = DirConfig.NewPrefix;

        for (ui32 suffix = 0; suffix < Archives.size(); ++suffix) {
            ctx.Portions.push_back(DirConfig.TempPrefix + ToString(suffix) + "_");
        }

        MergePortions(ctx);
        RemapDocIds(remapTable);
    }

    using TFactory =  NObjectFactory::TParametrizedObjectFactory<ITextArchiveBuilder, EArchiveType, TConstructContext>;

    ITextArchiveBuilder(const TDirConf& dirConf)
        : DirConfig(dirConf)
    {}

    virtual ~ITextArchiveBuilder() {};

protected:
    class IArcPortionWriter {
    public:
        virtual void Write(ui32 docId, TBlob docArchive) = 0;
        virtual void WriteTextArchiveHeader() = 0;
        virtual void Finish() = 0;
        virtual ~IArcPortionWriter() {};
    };

    struct TMergerContext {
        TString   OutputPath;
        TVector<TString> Portions;
        const TVector<ui32>* Remap;

        TMergerContext(const TVector<ui32>* remap)
            : Remap(remap)
        {}
    };

private:
    virtual void RemapDocIds(const TVector<ui32>* remap) = 0;
    virtual void MergePortions(const TMergerContext&) = 0;

protected:
    TVector<TAtomicSharedPtr<IArcPortionWriter>> Archives;
    TDirConf DirConfig;
};
