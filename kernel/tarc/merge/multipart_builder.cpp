#include "multipart_builder.h"

#include <library/cpp/logger/global/global.h>
#include <kernel/multipart_archive/multipart.h>


class TMultipartArchiveBuilder::TArcPortionWriter : public ITextArchiveBuilder::IArcPortionWriter {
public:
    TArcPortionWriter(const TString& arcPath, ui32 docsCount, const NRTYArchive::TMultipartConfig& multipartConfig) {
        using namespace NRTYArchive;
        Archive = TArchiveOwner::Create(arcPath, multipartConfig, docsCount);
    }

    void Write(ui32 docId, TBlob docArchive) override {
        Archive->PutDocument(docArchive, docId);
    }

    void WriteTextArchiveHeader() override {}

    void Finish() override {
        Archive->Flush();
        Archive.Reset(nullptr);
    }

private:
    TArchiveOwner::TPtr Archive;
};

TMultipartArchiveBuilder::TMultipartArchiveBuilder(const TConstructContext& ctx)
    : ITextArchiveBuilder(ctx.DirConfig)
    , MultipartConfig(ctx.MultipartConfig)
{
    for (ui32 i = 0;  i < ctx.ThreadsCount; ++i) {
        const TString& arcPath = DirConfig.TempPrefix + ToString(i) + "_arc";
        Archives.push_back(new TArcPortionWriter(arcPath, ctx.DocsCount, MultipartConfig));
    }
}

void TMultipartArchiveBuilder::MergePortions(const TMergerContext& ctx) {
    using namespace NRTYArchive;
    auto mergedOut = TArchiveOwner::Create(ctx.OutputPath + "arc", MultipartConfig);
    for (const auto& arcPart : ctx.Portions) {
        const TString arcPath = arcPart + "arc";
        auto oneArc = TArchiveOwner::Create(arcPath, MultipartConfig);
        VERIFY_WITH_LOG(oneArc.Get(), "can't find portion %s", arcPath.data());

        mergedOut->Append(*oneArc.Get(), nullptr, false);
        oneArc.Drop();
        TArchiveOwner::Remove(arcPath);
    }
}

void TMultipartArchiveBuilder::RemapDocIds(const TVector<ui32>* remap) {
    if (!remap)
        return;

    using namespace NRTYArchive;
    auto mergedOut = TArchiveOwner::Create(DirConfig.NewPrefix + "arc", MultipartConfig);
    mergedOut->Remap(*remap);
}

ITextArchiveBuilder::TFactory::TRegistrator<TMultipartArchiveBuilder> TMultipartArchiveBuilder::Registrator(AT_MULTIPART);
