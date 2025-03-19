#include "dater_simple.h"
#include "text_concatenator.h"

namespace ND2 {

void TDater::Localize(ELanguage lang, ELanguage auxlang, ECountryType ct) {
    Ctx.Document.MainLanguage = lang;
    Ctx.Document.AuxLanguage = auxlang;
    Ctx.Document.CountryType = ct;
}

void TDater::Clear() {
    InputText.Clear();
    InputTokenPositions.clear();
    OutputDates.clear();
    Ctx.Clear();
}

void TDater::Scan() {//EScanParams sp) {
    using namespace NSegm;

    Ctx.Reset();

    if (InputTokenPositions.empty())
        InputTokenPositions.push_back(TPosCoord(TAlignedPosting(1, 1), 0, InputText.size()));

    if (InputTokenPositions.front().Begin) {
        const TPosCoord& pc = InputTokenPositions.front();
        Ctx.Document.EventStorer.OnSpaces(true, ST_NOBRK, InputText.data(), pc.Begin, pc.Pos);
    }

    for (NSegm::TPosCoords::const_iterator it = InputTokenPositions.begin(); it != InputTokenPositions.end(); ++it) {
        Ctx.Document.EventStorer.OnSubToken(true, InputText.begin() + it->Begin, it->End - it->Begin, it->Pos);

        if (it + 1 != InputTokenPositions.end()) {
            const TPosCoord& next = *(it + 1);

            if (next.Begin > it->End) {
                bool nextsent = next.Pos.Sent() > it->Pos.Sent();
                ESpaceType st = nextsent ? ST_SENTBRK : ST_NOBRK;
                Ctx.Document.EventStorer.OnSpaces(true, st,
                                                  InputText.begin() + it->End, next.Begin - it->End, it->Pos);
            }
        } else if (it->End < (ptrdiff_t)InputText.size()) {
            Ctx.Document.EventStorer.OnSpaces(true, ST_SENTBRK,
                                              InputText.begin() + it->End, InputText.size() - it->End, it->Pos);
        }
    }

    Ctx.Document.EventStorer.OnTextEnd(InputTokenPositions.back().Pos.NextSent());
    Ctx.ScanTextRange(Ctx.Document.GetTitle(), OutputDates, true); //sp);
}

}
