#pragma once

#include <kernel/snippets/iface/archive/viewer.h>

#include <kernel/tarc/docdescr/docdescr.h>

#include <util/generic/string.h>

class IOutputStream;

namespace NSnippets
{
    class TPassageReply;
    class TSnipTitle;
    class IRestr;
    class TSnip;
    class TReplaceResult;
    class TSentsMatchInfo;
    class TMarkersMask;
    struct TEnhanceSnippetConfig;

    enum ECandidateSource {
        CS_TEXT_ARC = 0,
        CS_LINK_ARC,
        CS_METADATA,
        CS_COUNT
    };

    enum ETitleCandidateSource {
        TS_NATURAL,
        TS_HEADER_BASED,
        TS_NATURAL_WITH_HEADER_BASED
    };

    struct IAlgoTop {
        virtual void Push(const TSnip& snip, const TUtf16String& title = TUtf16String()) = 0;
        virtual ~IAlgoTop() {
        }
    };

    struct ISnippetTextDebugHandler : IArchiveViewer {
        virtual void AddHits(const TSentsMatchInfo& /*matchInfo*/) {
        }
        virtual void AddRestrictions(const char* /*restrType*/, const IRestr& /*restr*/) {
        }
        virtual void MarkFinalSnippet(const TSnip& /*finalSnippet*/) {
        }
        ~ISnippetTextDebugHandler() override {
        }
    };

    struct ISnippetCandidateDebugHandler {
        virtual IAlgoTop* AddTop(const char* algo, ECandidateSource src) = 0;
        virtual void AddTitleCandidate(const TSnipTitle& /*title*/, ETitleCandidateSource /*source*/) {
        }
        virtual ~ISnippetCandidateDebugHandler()
        {
        }
    };

    struct ISnippetDebugOutputHandler {
        virtual void Y_PRINTF_FORMAT(3,4) Print(bool important, const char* text, ...) = 0;
        virtual void ReplacerPrint(const TString& replacerName, const TReplaceResult& replaceResult, const TString& comment) = 0;
        virtual ~ISnippetDebugOutputHandler()
        {
        }
    };

    struct ISnippetsCallback {
        virtual ISnippetTextDebugHandler* GetTextHandler(bool /*isByLink*/) {
            return nullptr;
        }
        virtual ISnippetCandidateDebugHandler* GetCandidateHandler() {
            return nullptr;
        }
        virtual ISnippetDebugOutputHandler* GetDebugOutput() {
            return nullptr;
        }
        virtual double GetShareOfTrashCandidates(bool) const {
            return 0.0;
        }
        virtual void GetExplanation(IOutputStream& output) const = 0;
        virtual ~ISnippetsCallback() {
        }
        virtual void OnTitleSnip(const TSnip& /*natural*/, const TSnip& /*unnatural*/, bool /*isByLink*/){
        }
        virtual void OnBestFinal(const TSnip& /*snip*/, bool /*isByLink*/) {
        }
        virtual void OnTitleReplace(bool /*replace*/) {
        }
        virtual void OnPassageReply(const TPassageReply& /*reply*/, const TEnhanceSnippetConfig& /*cfg*/) {
        }
        virtual void OnDocInfos(const TDocInfos& /*docInfos*/) {
        }
        virtual void OnMarkers(const TMarkersMask& /*markers*/) {
        }
    };

    struct TSnippetsCallbackStub: public ISnippetsCallback {
        void GetExplanation(IOutputStream& /*output*/) const override {
        }
    };
}
