#pragma once

#include <library/cpp/regex/pire/pire.h>

#include <util/datetime/base.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <utility>

template <class ClassType>
class TRegexpClassifier {
public:
    struct TClassDescription {
        TString RegExp;
        ClassType Class;

        TClassDescription(const TString& regExp, ClassType classType)
            : RegExp(regExp)
            , Class(classType)
        {
        }
    };

    template <class Iter>
    TRegexpClassifier(Iter first, Iter last, ClassType defaultValue)
        : Descriptions(first, last)
        , DefaultValue(defaultValue)
    {
        using NPire::TLexer;
        using NPire::TScanner;

        if (Descriptions.size() == 0) {
            return;
        }

        Scanners.reserve(Descriptions.size());
        for (size_t i = 0; i < Descriptions.size(); i++) {
            Scanners.emplace_back(TLexer(Descriptions[i].RegExp).Parse().Compile<TScanner>(), i);
        }

        bool gluedSomething = true;

        while (gluedSomething && Scanners.size() > 1) {
            gluedSomething = false;

            size_t readIndex = 0;
            size_t writeIndex = 0;

            while (readIndex + 1 < Scanners.size()) {
                TScanner glued = TScanner::Glue(Scanners[readIndex].first, Scanners[readIndex + 1].first);

                if (glued.Empty()) {
                    Scanners[writeIndex] = Scanners[readIndex];
                    Scanners[writeIndex + 1] = Scanners[readIndex + 1];
                    writeIndex += 2;
                } else {
                    Scanners[writeIndex] = std::make_pair(glued, Scanners[readIndex].second);
                    writeIndex++;
                    gluedSomething = true;
                }

                readIndex += 2;
            }

            if (readIndex + 1 == Scanners.size()) {
                Scanners[writeIndex] = Scanners[readIndex];
                writeIndex++;
            }

            Scanners.resize(writeIndex);
        }
    }

    TRegexpClassifier(std::initializer_list<TClassDescription> list, ClassType defaultValue)
        : TRegexpClassifier(std::begin(list), std::end(list), defaultValue)
    {
    }

    ClassType operator[](const TStringBuf& urlPath) const {
        using NPire::Runner;
        using NPire::TScanner;

        for (size_t i = 0; i < Scanners.size(); ++i) {
            const TScanner& sc = Scanners[i].first;
            TScanner::State state = Runner(sc).Run(urlPath.begin(), urlPath.end()).State();
            if (sc.Final(state)) {
                std::pair<const size_t*, const size_t*> range = sc.AcceptedRegexps(state);
                size_t matchedDescription = *std::min_element(range.first, range.second);
                matchedDescription += Scanners[i].second;
                return Descriptions[matchedDescription].Class;
            }
        }
        return DefaultValue;
    }

    size_t GetScannersSize() const {
        return Scanners.size();
    }

private:
    const TVector<TClassDescription> Descriptions;
    ClassType DefaultValue;
    TVector<std::pair<NPire::TScanner, size_t>> Scanners;
};
