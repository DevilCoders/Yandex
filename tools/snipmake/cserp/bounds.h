#pragma once

namespace NSnippets {
    struct TBounds {
        int Top = 0;
        int Left = 0;
        int Width = 0;
        int Height = 0;
        int Bottom = 0;
        int Right = 0;

        bool operator==(const TBounds& other) const {
            return Top == other.Top && Left == other.Left && Width == other.Width && Height == other.Height && Bottom == other.Bottom && Right == other.Right;
        }
        bool operator!=(const TBounds& other) const {
            return !(*this == other);
        }
    };
}
