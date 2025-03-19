#pragma once

/*
 * Containts realization of different decomposition algorithms and visualization tool
 */

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <utility>

#include "common.h"

namespace NLexicalDecomposition {
    class TDecompositionResultDescr {
    public:
        static const ui8 MAX_BADNESS = 3;

        enum TDescrResult {
            DR_EMPTY = 0, /// don`t contain any result
            DR_GENERAL,   /// the result was obtained by DoDecomposition

            /*
             * Remaining results correspond to the manual decomposition;
             * notations: V - not a stop word, S - stop word, W - both vocabulary and stop word
             */
            DR_VSS,
            DR_VVS,
            DR_SVV,
            DR_VVV,
            DR_VSV,
            DR_WW,
            DR_W,
        };

    public:
        TDecompositionResultDescr(TDescrResult descr = DR_EMPTY, ui32 frequency = 0, ui32 badness = MAX_BADNESS + 1);

        bool IsWorseThan(const TDecompositionResultDescr& rhs) {
            return GetMeasure() < rhs.GetMeasure();
        }
        bool IsBetterThan(const TDecompositionResultDescr& rhs) {
            return GetMeasure() > rhs.GetMeasure();
        }
        bool Relax(const TDecompositionResultDescr&);

        ui64 GetMeasure() const {
            return Measure;
        }
        TDescrResult GetDescr() const {
            return (TDescrResult)((Measure >> 32) & Max<ui8>());
        }
        ui32 GetFrequency() const {
            return (Measure & Max<ui32>());
        }
        ui32 GetBadness() const {
            return ((Measure >> 40) ^ Max<ui8>());
        }

    private:
        ui64 Measure;
    };

    class TLexicalDecomposition {
    public:
        typedef TSimpleSharedPtr<TWordAdditionalInfoArr> TAdditionalInfoPtr;
        typedef TSimpleSharedPtr<TVector<ui32>> TEndposPtr;

    public:
        TLexicalDecomposition(ui32 options, ui32 tokenLength, const TEndposPtr& ends, const TAdditionalInfoPtr& additionalInfo,
                              const TDecompositionResultDescr& border = TDecompositionResultDescr());

        bool DoDecompositionManual(); /// true if found better result
        bool DoDecomposition();       /// true if found better result
        bool ResultIsUgly() const;
        bool WasImproved() const {
            return Improved;
        }
        const TDecompositionResultDescr& GetDescr() const {
            return Descr;
        }

        template <class TStringType>
        void SaveResult(const TStringType& token, TVector<TStringType>& toSave);

        template <class TStringType>
        void DoVisualize(const TStringType& token, IOutputStream& output);

    private:
        bool VocabularyElement(ui32 id) const {
            return (id < TokenAmount);
        }

        ui32 GetLength(ui32 id) const {
            return VocabularyElement(id) ? (*AdditionalInfo)[id]->Length : 0 - id;
        }

        ui32 GetEnd(ui32 index) const {
            return (*Ends)[index];
        }

        ui32 GetFreq(ui32 index) const {
            return (*AdditionalInfo)[index]->Frequency;
        }

        ui32 GetBadness(ui32 index) const {
            ui32 type = (*AdditionalInfo)[index]->Type;
            return (type == VWT_FREQ || type == VWT_UNKNOWN || type == VWT_STOP);
        }

        bool IsStopWord(ui32 index) const {
            return (*AdditionalInfo)[index]->Type == VWT_STOP;
        }

        void PushWord(ui32 id, ui32 length = 0) {
            Result.push_back(id != INFTY ? id : 0 - length);
        }

    private:
        ui32 Options;
        ui32 TokenLength;
        TEndposPtr Ends;
        TAdditionalInfoPtr AdditionalInfo;
        ui32 TokenAmount;

        TDecompositionResultDescr Descr;
        TVector<ui32> Result;
        bool Improved;
    };

}

#include "lexical_decomposition_algo_impl.h"
