#pragma once

#include "document.h"

namespace ND2 {
namespace NImpl {

struct TNormTextConcatenator {
    TUtf16String* Res;
    NSegm::TPosCoords* Coords;
    const TDaterDocumentContext* Document;

    NUF::TNormalizer Normalizer;

    TNormTextConcatenator();

    void SetContext(const TDaterDocumentContext*, TUtf16String* res, NSegm::TPosCoords* rescoords);

    void DoNormalize(TWtringBuf in);

    void DoNormalize(TWtringBuf in, TUtf16String& out);

    void Do(const NSegm::TRange& r);
};

}
}
