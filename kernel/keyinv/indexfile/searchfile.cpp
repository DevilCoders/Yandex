#include <util/generic/ptr.h>
#include "indexfile.h"
#include "searchfile.h"

using namespace NIndexerCore;

TYndex4Searching::TYndex4Searching() = default;

TYndex4Searching::~TYndex4Searching() = default;

const YxRecord* TYndex4Searching::EntryByNumber(TRequestContext &rc, i32 number, i32 &block) const {
    return GetEntryByNumber(Fat, IndexInfo, *KeyFile, rc, number, block);
}

const char *TYndex4Searching::WordByNumber(TRequestContext &rc, i32 number) const {
    i32 block = UNKNOWN_BLOCK;
    const YxRecord* entry = EntryByNumber(rc, number, block);
    if (!entry)
        return "???";
    return entry->TextPointer;
}

i32 TYndex4Searching::LowerBound(const char *word, TRequestContext &rc) const {
    return FatLowerBound(Fat, IndexInfo, *KeyFile, word, rc);
}

void TYndex4Searching::InitSearch(const TString& keyName, const TString& invName, READ_HITS_TYPE defaultReadHitsType/* = RH_DEFAULT*/) {
    TMemoryMap keyMapping(keyName);
    TMemoryMap invMapping(invName);
    InitSearch(keyMapping, invMapping, defaultReadHitsType);
}

void TYndex4Searching::InitSearch(const TMemoryMap& keyMapping, const TMemoryMap& invMapping, READ_HITS_TYPE defaultReadHitsType/* = RH_DEFAULT*/) {
    CloseSearch();

    TMemoryMapStream invStream(invMapping);
    ui32 version = 0;
    TInvKeyInfo invKeyInfo;
    ReadIndexInfoFromStream(invStream, version, invKeyInfo);
    InvFile.Reset(new TMemoryMap(invMapping));
    KeyFile.Reset(new TFileMap(keyMapping));
    IndexInfo = Fat.Open(invStream, version, invKeyInfo);
    InvFileName = invMapping.GetFile().GetName();
    DefaultReadHitsType = defaultReadHitsType;

    TRequestContext rc;
    i32 num = LowerBound("\x7f", rc);
    i32 block = UNKNOWN_BLOCK;
    const YxRecord *record = EntryByNumber(rc, num, block);
    IndexInfo.HasUtfKeys = record && record->TextPointer[0] == '\x7f';
}

void TYndex4Searching::InitSearch(const TString& indexName, READ_HITS_TYPE defaultReadHitsType/* = RH_DEFAULT*/) {
    TString key(TString::Join(indexName, KEY_SUFFIX));
    TString inv(TString::Join(indexName, INV_SUFFIX));
    InitSearch(key, inv, defaultReadHitsType);
}

void TYndex4Searching::CloseSearch() {
    Fat.Clear();
    InvFile.Destroy();
    KeyFile.Destroy();
}

#ifndef MIN_MAP
#   define MIN_MAP 0 /*Os::PAGESIZE*2 ?*/
#endif

#if defined(__hpux__)
#   define UseMap(n) (false)
#elif defined(_32_) || defined(_64_)
#   define UseMap(n) (n > MIN_MAP)
#else
#   define UseMap(n) (false)
#endif

void TYndex4Searching::GetBlob(TBlob& data, i64 offset, ui32 length, READ_HITS_TYPE type) const
{
    //printf("GetBlob %s %" PRIu64 " %" PRIu32 "\n",~InvFileName, offset, length);

    if (type == RH_DEFAULT) {
        //If user doesn`t specified read type use in class default
        type = DefaultReadHitsType;
    }

    if (type == RH_DEFAULT) {
        //By default use map
        type = RH_FORCE_MAP;
    }

    if (type == RH_FORCE_MAP && UseMap(length)) {
        data = TBlob::FromMemoryMap(*InvFile, offset, length);
    } else {
        TFile tempFile = InvFile->GetFile();
        assert(tempFile.IsOpen());
        data = TBlob::FromFileContent(tempFile, offset, length);
    }
}

i64 TYndex4Searching::GetInvLength() const {
    if (!InvFile)
        ythrow yexception() << "Inv file hasn't been initialized!";
    return InvFile->Length();
}
