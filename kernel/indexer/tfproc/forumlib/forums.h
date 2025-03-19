#pragma once

#include <library/cpp/numerator/numerate.h>
#include <util/stream/output.h>
#include <util/generic/hash_set.h>
#include <util/generic/list.h>
#include <util/memory/segmented_string_pool.h>
#include <kernel/indexer/tfproc/date_recognizer.h>
#include "forum_tag_chains.h"

class IDocumentDataInserter;

namespace NForumsImpl {
class TMainImpl;
}

class TForumsHandler : public INumeratorHandler
{
public:
    enum EGenerator
    {
        Unknown,
        ExBB,
        InvisionPB,
        PhpBB,
        PunBB,
        SimpleMachinesForum,
        UBBThreads,
        VBulletin,
        Ucoz,
        DonanimHaber,
    };
    // getters
    EGenerator GetGenerator() const;
    int GetNumPosts() const;
    const TRecognizedDate& GetFirstPostDate() const;
    const TRecognizedDate& GetLastPostDate() const;
    int GetNumDifferentAuthors() const;
    const char* GetGeneratorName() const
    {
        switch (GetGenerator()) {
        case Unknown:
            return "unknown";
        case ExBB:
            return "ExBB";
        case InvisionPB:
            return "InvisionPB";
        case PhpBB:
            return "phpBB";
        case PunBB:
            return "PunBB";
        case SimpleMachinesForum:
            return "SMF";
        case UBBThreads:
            return "UBB.threads";
        case VBulletin:
            return "vBulletin";
        case Ucoz:
            return "uCoz";
        case DonanimHaber:
            return "DonanimHaber";
        }
        return nullptr; // should not happen
    }
    // processors
    void Clear(void);
    void OnAddDoc(const char* /*url*/, time_t date, bool isDateAmericanHint);
    void OnAddDoc(const char* url, time_t date, ELanguage primaryLang)
    {
        OnAddDoc(url, date, primaryLang == LANG_ENG || primaryLang == LANG_TUR);
    }
    void OnCommitDoc(IDocumentDataInserter* inserter);
    void OnTextStart(const IParsedDocProperties* parser) override;
    void OnTextEnd(const IParsedDocProperties*, const TNumerStat&) override;
    void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry*, const TNumerStat& numerStat) override;
    void OnTokenStart(const TWideToken& token, const TNumerStat& numerStat) override;
    void OnSpaces(TBreakType, const wchar16 *token, unsigned len, const TNumerStat& numerStat) override;
    TForumsHandler();
    ~TForumsHandler() override;
private:
    THolder<NForumsImpl::TMainImpl> Impl;
};
