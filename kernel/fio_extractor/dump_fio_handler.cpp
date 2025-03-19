#include "dump_fio_handler.h"

#include <util/stream/str.h>
#include <util/system/maxlen.h>

namespace NFioExtractor {

    bool operator< (const TFioDumper::TPart& lhs, const TFioDumper::TPart& rhs) {
        return lhs.type < rhs.type || lhs.type == rhs.type && lhs.value < rhs.value;
    }

    void TFioDumper::StartFio() {
        Parts.clear();
        if (Handler)
            Handler->StartFio();
    }

    void TFioDumper::ProcessFioMember(const TUtf16String& lemma,
            ENameType nameType, TPosting pos, bool initials)
    {
        Parts.push_back(TPart { nameType, lemma });
        if (Handler)
            Handler->ProcessFioMember(lemma, nameType, pos, initials);
    }

    void TFioDumper::UpdateFioPos(TPosting begin, TPosting end) {
        if (Handler)
            Handler->UpdateFioPos(begin, end);
    }

    void TFioDumper::FinishFio() {
        Sort(Parts.begin(), Parts.end());
        TStringStream stream;
        stream << Prefix;
        typedef TVector<TPart>::const_iterator iterator;
        for (iterator part = Parts.begin(); part != Parts.end(); ++part) {
            stream << "#" << int(part->type) << ":" << part->value;
        }
        ++Counter[stream.Str()];
        if (Handler)
            Handler->FinishFio();
    }

    void TFioDumper::Dump(IOutputStream& result) const {
        typedef THashMap<TString, size_t>::const_iterator iterator;
        for (iterator it = Counter.begin(); it != Counter.end(); ++it) {
            result <<  it->first << "#"
                    << it->second << Endl;
        }
    }
}

