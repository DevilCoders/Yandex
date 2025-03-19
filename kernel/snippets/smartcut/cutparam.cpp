#include "cutparam.h"

namespace NSnippets
{
    TCutParams::TCutParams(bool isPixel, int pixelsInLine, float fontSize)
        : IsPixel(isPixel)
        , PixelsInLine(pixelsInLine)
        , FontSize(fontSize)
    {
    }

    TCutParams TCutParams::Symbol() {
        return TCutParams(false, 0, 0.0f);
    }

    TCutParams TCutParams::Pixel(int pixelsInLine, float fontSize) {
        return TCutParams(true, pixelsInLine, fontSize);
    }

} // namespace NSnippets
