#pragma once

namespace NLexicalDecomposition {
    template <class TStringType>
    void TLexicalDecomposition::SaveResult(const TStringType& token, TVector<TStringType>& toSave) {
        toSave.clear();
        for (size_t i = 0, pos = 0; i < Result.size(); ++i) {
            ui32 length = GetLength(Result[i]);
            if (VocabularyElement(Result[i]) || !(Options & DO_ORFO))
                toSave.push_back(token.substr(pos, length));
            pos += length;
        }
    }

    template <class TStringType>
    void TLexicalDecomposition::DoVisualize(const TStringType& token, IOutputStream& output) {
        output << token << ":";
        for (size_t i = 0, pos = 0; i < Result.size(); ++i) {
            ui32 length = GetLength(Result[i]);
            if (VocabularyElement(Result[i]))
                output << ' ' << token.substr(pos, length);
            pos += length;
        }
        output << Endl;

        for (size_t i = 0; i < Result.size(); ++i) {
            char c = VocabularyElement(Result[i]) ? '*' : '.';
            output << TString(GetLength(Result[i]), c);
        }
        output << Endl;
        output << "Descr:\t" << (ui32)Descr.GetDescr() << ", Frequency:\t" << Descr.GetFrequency() << ", Badness:\t" << Descr.GetBadness() << ", Mesure:\t" << Descr.GetMeasure() << Endl;
        output << Endl;
    }

}
