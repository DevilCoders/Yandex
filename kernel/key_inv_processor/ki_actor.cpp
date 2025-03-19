#include "ki_actor.h"

constexpr ui32 REMAP_NOWHERE = Max<ui32>();

void TRTYKIActor::Flush() {
    MergeTaskWriter.AddInput(Directory + "/" + Prefix + ToString(PortionNum) + "p", IYndexStorage::PORTION_FORMAT);
    PortionNum++;
}

TRTYKIActor::TRTYKIActor(const TString& prefix, const TString& dir, TAdvancedMergeTaskWriter& mergeTaskWriter, ui32 maxPortionDocs, const bool buildByKeysFlag)
    : Directory(dir)
    , Prefix(prefix)
    , PortionNum(0)
    , MaxPortionDocs(maxPortionDocs)
    , MergeTaskWriter(mergeTaskWriter)
    , Working(false)
    , BuildByKeysFlag(buildByKeysFlag)
{
}

void TRTYKIActor::Start() {
    VERIFY_WITH_LOG(!Working, "Incorrect KIActor usage");
    MPUAttr.Reset(new TMemoryPortionAttr(IYndexStorage::PORTION_FORMAT_ATTR, MaxPortionDocs, Prefix, "a", Directory, BuildByKeysFlag));
    MPULemm.Reset(new TMemoryPortionBody(IYndexStorage::PORTION_FORMAT_LEMM, MaxPortionDocs, Prefix, "l", Directory, BuildByKeysFlag));
    Working = true;
}

void TRTYKIActor::Stop() {
    INFO_LOG << "TRTYKIActor stopping" << Endl;
    VERIFY_WITH_LOG(Working, "Incorrect KIActor usage");
    Working = false;
}

bool TRTYKIActor::IncDoc() {
    VERIFY_WITH_LOG(Working, "Incorrect TRTYKIActor usage");
    bool aFlush = MPUAttr->IncDoc();
    bool lFlush = MPULemm->IncDoc();
    VERIFY_WITH_LOG(aFlush == lFlush, "Incorrect TRTYKIActor usage");
    if (aFlush && lFlush) {
        Flush();
    }
    return true;
}

void TRTYKIActor::Close() {
    VERIFY_WITH_LOG(!Working, "Incorrect KIActor usage");
    bool aClose = MPUAttr->Close();
    bool lClose = MPULemm->Close();
    VERIFY_WITH_LOG(!(aClose ^ lClose), "Incorrect TRTYKIActor usage");
    if (aClose && lClose)
        Flush();
    MPUAttr.Destroy();
    MPULemm.Destroy();
}


void TRTYKIActor::Discard() {
    if (MPUAttr)
        MPUAttr->Close();
    if (MPULemm)
        MPULemm->Close();
    MPUAttr.Destroy();
    MPULemm.Destroy();
}

bool TRTYKIActor::StoreDoc(IKIStorage& storage, ui32 docId) {
    VERIFY_WITH_LOG(Working, "Incorrect TRTYKIActor usage");
    const char* key;
    SUPERLONG* positions;
    ui32 posCount;
    while (storage.Next(key, positions, posCount)) {
        for (ui32 i = 0; i < posCount; ++i)
            TWordPosition::SetDoc(positions[i], docId);
        MPUAttr->StorePositions(key, positions, posCount);
        MPULemm->StorePositions(key, positions, posCount);
    }
    bool aFlush = MPUAttr->IncDoc();
    bool lFlush = MPULemm->IncDoc();
    VERIFY_WITH_LOG(aFlush == lFlush, "Incorrect TRTYKIActor usage");
    return true;
}

bool TRTYKIActor::StorePositions(const char* key, SUPERLONG* positions, ui32 posCount) {
    VERIFY_WITH_LOG(Working, "Incorrect TRTYKIActor usage");
    MPUAttr->StorePositions(key, positions, posCount);
    MPULemm->StorePositions(key, positions, posCount);
    return true;
}

TAdvancedMergeTaskWriter::TAdvancedMergeTaskWriter()
    : Task(MakeSimpleShared<TAdvancedMergeTask>())
{
}

TAdvancedMergeTaskWriter::~TAdvancedMergeTaskWriter() {
}

void TAdvancedMergeTaskWriter::AddInput(const TString& prefix, IYndexStorage::FORMAT format) {
    TAdvancedMergeTask::TMergeInput input(prefix, format);
    input.Version = YNDEX_VERSION_PORTION_DEFAULT;
    TGuard<TMutex> g(Mutex);
    Task->Inputs.push_back(input);
}

void TAdvancedMergeTaskWriter::AddOutput(const TString& prefix) {
    TAdvancedMergeTask::TMergeOutput output(prefix);
    TGuard<TMutex> g(Mutex);
    Task->Outputs.push_back(output);
}

void TAdvancedMergeTaskWriter::BuildRemapTable(const TVector<ui32>* remapTable) {
    if (remapTable) {
        Task->FinalRemapTable.Create(1, 0);
        for (ui32 i = 0; i < remapTable->size(); ++i) {
            if ((*remapTable)[i] != REMAP_NOWHERE)
                Task->FinalRemapTable.SetNewDocId(0, i, (*remapTable)[i]);
            else
                Task->FinalRemapTable.SetDeletedDoc(0, i);
        }
    }
}
