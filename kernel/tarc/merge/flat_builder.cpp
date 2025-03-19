#include "flat_builder.h"
#include "merge.h"

#include <kernel/tarc/iface/tarcio.h>
#include <kernel/tarc/merge/builder.h>

#include <library/cpp/logger/global/global.h>
#include <library/cpp/deprecated/mbitmap/mbitmap.h>

#include <util/stream/file.h>
#include <util/ysaveload.h>

class TFlatArchiveBuilder::TArcPortionWriter : public ITextArchiveBuilder::IArcPortionWriter {
public:
    TArcPortionWriter(const TString& arcPath, const TString& dirPath, ui32 version)
        : ArcStream(CreateOutput(arcPath))
        , DirBuilder(new TArchiveDirBuilder(THolder<IOutputStream>(CreateOutput(dirPath))))
        , Version(version)
    {}

    void Write(ui32 docId, TBlob docArchive) override {
        TArchiveHeader* header = (TArchiveHeader*) docArchive.Data();
        header->DocId = docId;
        ArcStream->Write(docArchive.Data(), docArchive.Size());
        DirBuilder->AddEntry(*header);
    }

    void WriteTextArchiveHeader() override {
        ::WriteTextArchiveHeader(*ArcStream, Version);
    }

    void Finish() override {
        ArcStream->Finish();
        DirBuilder->Finish();

        ArcStream.Destroy();
        DirBuilder.Destroy();
    }

private:
    inline static TFileOutput* CreateOutput(const TString& path) {
        TFileOutput* stream = new TFileOutput(path);
        stream->SetFinishPropagateMode(true); // declared as 'noexcept'
        return stream;
    }

private:
    THolder<TFileOutput> ArcStream;
    THolder<TArchiveDirBuilder> DirBuilder;
    const ui32 Version;
};

TFlatArchiveBuilder::TFlatArchiveBuilder(const TConstructContext& ctx)
    : ITextArchiveBuilder(ctx.DirConfig)
    , ArchiveVersion(ctx.ArchiveVersion)
{
    for (ui32 i = 0;  i < ctx.ThreadsCount; ++i) {
        const TString& arcPath = DirConfig.TempPrefix + ToString(i) + "_arc";
        const TString& dirPath = DirConfig.TempPrefix + ToString(i) + "_dir";
        Archives.push_back(new TArcPortionWriter(arcPath, dirPath, ArchiveVersion));
    }
}

void TFlatArchiveBuilder::MergePortions(const TMergerContext& ctx) {
    if (Archives.size() == 1) {
        TFsPath(ctx.Portions[0] + "arc").ForceRenameTo(DirConfig.NewPrefix + "arc.tmp");
        TFsPath(ctx.Portions[0] + "dir").ForceRenameTo(DirConfig.NewPrefix + "dir.tmp");
    } else {
        bitmap_2 addDelDocuments;
        TVector<ui32> docs;
        for (size_t i = 0; i < ctx.Portions.size(); i++) {
            TFsPath plsadPath(ctx.Portions[i] + "plsad");
            TFileInput f(plsadPath.GetPath());
            ::Load(&f, docs);
            for (size_t j = 0; j < docs.size(); j++) {
                addDelDocuments.set(docs[j], MM_APPEND);
            }
            docs.clear();
        }

        MergeArchives("arc", ctx.Portions, "", ctx.OutputPath,
                MM_DEFAULT, &addDelDocuments, 0, ArchiveVersion, ctx.Remap->data(), ctx.Remap->size());
        MakeArchiveDir((ctx.OutputPath + "arc").data(), (ctx.OutputPath + "dir").data());
    }

    for (const auto& arcPortion : ctx.Portions) {
        TFsPath(arcPortion + "arc").DeleteIfExists();
        TFsPath(arcPortion + "dir").DeleteIfExists();
    }
}

void TFlatArchiveBuilder::RemapDocIds(const TVector<ui32>* remap) {
    if (Archives.size() != 1)
        return;

    if (remap) {
        TArchiveIterator arcIter;
        arcIter.Open((DirConfig.NewPrefix + "arc.tmp").data());

        const TString arcPath = DirConfig.NewPrefix + "arc";
        const TString dirPath = DirConfig.NewPrefix + "dir";
        TArcPortionWriter finalArc(arcPath, dirPath, ArchiveVersion);
        finalArc.WriteTextArchiveHeader();

        for (TArchiveHeader* curHdr = arcIter.NextAuto(); curHdr; curHdr = arcIter.NextAuto()) {
            size_t size = curHdr->SizeOf();
            ui32 tmpDocId = curHdr->DocId;
            if (tmpDocId < remap->size()) {
                ui32 finalDocId = (*remap)[tmpDocId];
                if (finalDocId < DeletedDocument()) {
                    finalArc.Write(finalDocId, TBlob::NoCopy(reinterpret_cast<const char*>(curHdr), size));
                }
            }
        }
        finalArc.Finish();

        TFsPath(DirConfig.NewPrefix + "arc.tmp").DeleteIfExists();
        TFsPath(DirConfig.NewPrefix + "dir.tmp").DeleteIfExists();
    } else {
        TFsPath(DirConfig.NewPrefix + "arc.tmp").RenameTo(DirConfig.NewPrefix + "arc");
        TFsPath(DirConfig.NewPrefix + "dir.tmp").RenameTo(DirConfig.NewPrefix + "dir");
    }
}

ITextArchiveBuilder::TFactory::TRegistrator<TFlatArchiveBuilder> TFlatArchiveBuilder::Registrator(AT_FLAT);
