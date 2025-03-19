#pragma once

#include <kernel/snippets/config/enums.h>

#include <util/generic/string.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>

class TArchiveMarkupZones;

namespace NSnippets
{
    //forward declarations
    class TConfig;
    class TQueryy;
    class TStatAnnotViewer;
    class TSnipTitle;
    class TArchiveView;
    class IArchiveViewer;
    class TArchiveMarkup;

    enum EStatAnnotType {
        SAT_UNKNOWN,
        SAT_MAIN_CONTENT,
        SAT_FIRST_CONTENT,
        SAT_FIRST_REFERAT,
        SAT_DOC_START_IF_NO_SEGMENTS,
    };

    class TStaticAnnotation : private TNonCopyable
    {
    public:
        TStaticAnnotation(const TConfig& cfg, const TArchiveMarkup& markup);
        ~TStaticAnnotation();

    public:
        TUtf16String GetSpecsnippet() const;
        EStatAnnotType GetStatAnnotType() const;

        void InitFromSentenceViewer(const TStatAnnotViewer& sentenceViewer, const TSnipTitle& title, const TQueryy& query);

    private:
        class TImpl;
        THolder<TImpl> Impl;
    };


    class TStatAnnotViewer {
    public:
        TStatAnnotViewer(const TConfig& cfg, const TArchiveMarkup& markup);
        ~TStatAnnotViewer();

    public:
        IArchiveViewer& GetViewer();
        const TArchiveView& GetResult() const;
        EStatAnnotType GetResultType() const;

    private:
        class TImpl;
        THolder<TImpl> Impl;
    };

}
