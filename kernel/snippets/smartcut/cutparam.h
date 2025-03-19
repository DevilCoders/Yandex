#pragma once

namespace NSnippets
{
    class TCutParams {
    public:
        bool IsPixel = false;
        int PixelsInLine = 0;
        float FontSize = 0.0f;
    public:
        TCutParams(bool isPixel, int pixelsInLine, float fontSize);
        static TCutParams Symbol();
        static TCutParams Pixel(int pixelsInLine, float fontSize);
    };
}
