#include "replace.h"
#include <kernel/snippets/sent_match/retained_info.h>
#include <kernel/snippets/sent_match/extra_attrs.h>

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/sent_match/callback.h>

#include <library/cpp/html/pcdata/pcdata.h>

#include <util/generic/vector.h>

namespace NSnippets {

    class TReplaceManager::TImpl {
    public:
        THolder<const TReplaceContext> RepCtx;
        TVector<TAutoPtr<IReplacer>> Replacers;
        TVector<TAutoPtr<IReplacer>> PostReplacers;
        bool IsExecuted = false;
        bool IsReplacePerformed = false;
        TString ReplacerUsed;
        TString CurrentReplacerName;
        TReplaceResult Result;
        TExtraSnipAttrs& ExtraSnipAttrs;
        ISnippetsCallback& Callback;
        TTopCandidateCallback* FactSnippetTopCandidatesCallback;

        TCustomSnippetsStorage CustomSnippets;

        TImpl(TAutoPtr<const TReplaceContext> repCtx, TExtraSnipAttrs& extraSnipAttrs, ISnippetsCallback& callback, TTopCandidateCallback* fsCallback)
            : RepCtx(repCtx)
            , ExtraSnipAttrs(extraSnipAttrs)
            , Callback(callback)
            , FactSnippetTopCandidatesCallback(fsCallback)
        {
        }
    };

    TReplaceManager::TReplaceManager(TAutoPtr<const TReplaceContext> repCtx, TExtraSnipAttrs& extraSnipAttrs, ISnippetsCallback& callback, TTopCandidateCallback* fsCallback)
      : Impl(new TImpl(repCtx, extraSnipAttrs, callback, fsCallback))
    {
    }

    TReplaceManager::~TReplaceManager() {
    }

    ISnippetsCallback& TReplaceManager::GetCallback() {
        return Impl->Callback;
    }

    TTopCandidateCallback* TReplaceManager::GetFactSnippetTopCandidatesCallback() {
        return Impl->FactSnippetTopCandidatesCallback;
    }

    TExtraSnipAttrs& TReplaceManager::GetExtraSnipAttrs() {
        return Impl->ExtraSnipAttrs;
    }

    const TReplaceResult& TReplaceManager::GetResult() const {
        return Impl->Result;
    }

    const TReplaceContext& TReplaceManager::GetContext() const {
        return *Impl->RepCtx;
    }

    TCustomSnippetsStorage& TReplaceManager::GetCustomSnippets() {
        return Impl->CustomSnippets;
    }

    void TReplaceManager::AddReplacer(TAutoPtr<IReplacer> replacer) {
        Impl->Replacers.push_back(replacer);
    }

    void TReplaceManager::AddPostReplacer(TAutoPtr<IReplacer> postReplacer) {
        Impl->PostReplacers.push_back(postReplacer);
    }

    void TReplaceManager::ReportReplacerName() {
        Impl->ReplacerUsed = Impl->CurrentReplacerName;
        if (Impl->Callback.GetDebugOutput()) {
            Impl->Callback.GetDebugOutput()->Print(true, "Replacer used: %s", EncodeHtmlPcdata(Impl->ReplacerUsed, true).data());
        }
    }

    void TReplaceManager::Commit(const TReplaceResult& newResult, const EMarker marker) {
        if (marker < MRK_COUNT)
            Impl->ExtraSnipAttrs.Markers.SetMarker(marker);
        Impl->IsReplacePerformed = true;
        Impl->Result = newResult;

        if (!Impl->Result.GetTitle())
            Impl->Result.UseTitle(Impl->RepCtx->NaturalTitle);
        Impl->ExtraSnipAttrs.SetSpecAttrs(Impl->Result.GetSpecSnipAttrs());
        ReplacerDebug("Success", newResult);
    }

    void TReplaceManager::DoWork() {
        if (Impl->IsExecuted) {
            ythrow yexception() << "ReplaceManager already executed.";
        }
        Y_ASSERT(Impl->RepCtx);

        if (!Impl->RepCtx->Cfg.BypassReplacers()) {
            for (auto&& replacer : Impl->Replacers) {
                Impl->CurrentReplacerName = replacer->GetReplacerName();

                if (Impl->RepCtx->Cfg.SwitchOffReplacer(Impl->CurrentReplacerName)) {
                    continue;
                }

                replacer->DoWork(this);
                if (Impl->IsReplacePerformed) {
                    ReportReplacerName();
                    break;
                }
            }
        }

        Impl->IsExecuted = true;

        for (auto&& postReplacer : Impl->PostReplacers) {
            if (Impl->RepCtx->Cfg.SwitchOffReplacer(postReplacer->GetReplacerName())) {
                continue;
            }
            postReplacer->DoWork(this);
        }
    }

    bool TReplaceManager::IsReplaced() const {
        if (!Impl->IsExecuted) {
            ythrow yexception() << "Call DoWork() first.";
        }
        return Impl->IsReplacePerformed;
    }

    bool TReplaceManager::IsBodyReplaced() const {
        return IsReplaced() && Impl->Result.CanUse();
    }

    void TReplaceManager::ReplacerDebug(const TString& comment) {
        if (Impl->Callback.GetDebugOutput()) {
            Impl->Callback.GetDebugOutput()->ReplacerPrint(Impl->CurrentReplacerName, TReplaceResult(), comment);
        }
    }

    void TReplaceManager::ReplacerDebug(const TString& comment, const TReplaceResult& result) {
        if (Impl->Callback.GetDebugOutput()) {
            Impl->Callback.GetDebugOutput()->ReplacerPrint(Impl->CurrentReplacerName, result, comment);
        }
    }

    const TString& TReplaceManager::GetReplacerUsed() {
        return Impl->ReplacerUsed;
    }

    void TReplaceManager::SetMarker(const EMarker marker, bool value) {
        if (marker < MRK_COUNT)
            GetExtraSnipAttrs().Markers.SetMarker(marker, value);
    }
}
