#include "ki_maker.h"

#include <kernel/index_mapping/index_mapping.h>
#include <kernel/walrus/advmerger.h>

void TRTYKIReader::Open() {
    NOTICE_LOG << "KeyInv reader for " << FilePath << " opening - " << (ui64)this << Endl;
    VERIFY_WITH_LOG(!YR, "Incorrect TRTYKIReader usage");
    if (LockFiles) {
        TIndexPrefetchOptions opts;
        opts.TryLock = true;
        EnableGlobalIndexFileMapping();
        CHECK_WITH_LOG(PrefetchMappedIndex(FilePath + "key", opts).Locked);
        CHECK_WITH_LOG(PrefetchMappedIndex(FilePath + "inv", opts).Locked);
    };
    YR.Reset(new TTextRequester(FilePath.data()));
    NOTICE_LOG << "KeyInv reader for " << FilePath << " opened - " << (ui64)this << Endl;
}

void TRTYKIReader::Close() {
    NOTICE_LOG << "KeyInv reader for " << FilePath << " closing - " << (ui64)this << Endl;
    VERIFY_WITH_LOG(!!YR, "Incorrect TRTYKIReader usage");
    YR.Destroy();
    // TTextRequester uses global mapping, so we have to release it here
    ReleaseMappedIndexFile(FilePath + "key");
    ReleaseMappedIndexFile(FilePath + "inv");
    NOTICE_LOG << "KeyInv reader for " << FilePath << " closed - " << (ui64)this << Endl;
}

bool TRTYKIMaker::IncDoc(ui32 actorId) {
    VERIFY_WITH_LOG(KIActors.size() > actorId, "Incorrect TRTYKIMaker usage");
    return KIActors[actorId]->IncDoc();
}

void TRTYKIMaker::Stop() {
    for (ui32 i = 0; i < ActorsCount; ++i) {
        KIActors[i]->Stop();
    }
}

void TRTYKIMaker::Start() {
    TString prefix = Dir + "/" + Prefix;
    for (ui32 i = 0; i < ActorsCount; ++i) {
        KIActors.push_back(new TRTYKIActor(Prefix + "." + ToString(i) + "_", Dir, MergeTaskWriter, MaxPortionDocs, BuildByKeysFlag));
        KIActors.back()->Start();
    }
}

void TRTYKIMaker::Close(const TVector<ui32>* remapTable) {
    for (ui32 i = 0; i < ActorsCount; ++i) {
        KIActors[i]->Close();
    }
    KIActors.clear();
    MergeTaskWriter.AddOutput(Dir + "/" + Prefix);
    MergeTaskWriter.BuildRemapTable(remapTable);
    {
        TAdvancedIndexMerger aim(MergeTaskWriter.GetTask());
        if (aim.Run()) {
            for (ui32 i = 0; i < MergeTaskWriter.GetTask()->Inputs.size(); ++i) {
                NFs::Remove(MergeTaskWriter.GetTask()->Inputs[i].FilePrefix + "ak");
                NFs::Remove(MergeTaskWriter.GetTask()->Inputs[i].FilePrefix + "ai");
                NFs::Remove(MergeTaskWriter.GetTask()->Inputs[i].FilePrefix + "lk");
                NFs::Remove(MergeTaskWriter.GetTask()->Inputs[i].FilePrefix + "li");
            }
        } else {
            ythrow yexception() << "Incorrect advanced merger behavior";
        }
    }
}

void TRTYKIMaker::Discard() {
    for (auto& actor : KIActors) {
        actor->Discard();
    }
    KIActors.clear();
}
