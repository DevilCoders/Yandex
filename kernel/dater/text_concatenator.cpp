#include "text_concatenator.h"

namespace ND2 {
namespace NImpl {

using NSegm::TRange;
using NSegm::TRanges;
using NSegm::TSegEvent;
using NSegm::TPosCoord;
using NSegm::TPosCoords;

using NSegm::SET_TOKEN;
using NSegm::SET_SPACE;
using NSegm::SET_PARABRK;

TNormTextConcatenator::TNormTextConcatenator()
    : Res()
    , Coords()
    , Document()
{
    Normalizer.SetDoLowerCase(true);
    Normalizer.SetDoRenyxa(true);
    Normalizer.SetDoSimpleCyr(true);
}

void TNormTextConcatenator::SetContext(const TDaterDocumentContext* ctx, TUtf16String* res, TPosCoords* pos) {
    Document = ctx;
    Res = res;
    Coords = pos;
}

void TNormTextConcatenator::DoNormalize(TWtringBuf in) {
    Normalizer.SetInput(in);
    Normalizer.DoNormalize();
}

void TNormTextConcatenator::DoNormalize(TWtringBuf in, TUtf16String& out) {
    DoNormalize(in);
    TWtringBuf b = Normalizer.GetOutput();
    out.assign(b.data(), b.size());
}

inline void PushPos(TPosCoords* coords, NSegm::TAlignedPosting pos, const TUtf16String* res, size_t diff) {
    if (!coords->empty() && coords->back().Pos == pos) {
        coords->back().End += diff;
    } else {
        size_t b = coords->empty() ? res->size() : coords->back().End;
        coords->push_back(TPosCoord(pos, b, b + diff));
        Y_VERIFY(b == res->size(), " ");
    }
}

void TNormTextConcatenator::Do(const TRange& r) {
    static const wchar16 sp[] = {' ', 0};

    if (r.empty())
        return;

    Coords->reserve(r.size());
    Res->reserve(20 * r.size());

    Res->append(' ');

    for (const TSegEvent* s = r.begin(); s != r.end(); ++s) {
        if (s->IsA<SET_SPACE>() && !s->Text /* fake space */) {
            if (Res->EndsWith(sp, 1))
                continue;

            PushPos(Coords, s->Pos, Res, 1);
            Res->append(' ');
        } else if (s->IsA<SET_TOKEN>() || s->IsA<SET_SPACE>()) {
            DoNormalize(s->Text);

            TWtringBuf b = Normalizer.GetOutput();

            if (Res->EndsWith(sp, 1))
                while (b.StartsWith(sp, 1))
                    b.Skip(1);

            if (b.empty())
                continue;

            PushPos(Coords, s->Pos, Res, b.size());
            Res->append(b.data(), b.size());
        }
    }
}

}
}
