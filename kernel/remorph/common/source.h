#pragma once

#include <util/generic/strbuf.h>
#include <util/stream/output.h>

namespace NReMorph {

struct TSourcePos {
    size_t Line;
    size_t Column;

    explicit TSourcePos()
        : Line(0)
        , Column(0)
    {
    }

    explicit TSourcePos(size_t line, size_t column)
        : Line(line)
        , Column(column)
    {
    }

    inline TSourcePos& operator+=(const TSourcePos& other) {
        Line += other.Line;
        if (!other.Line) {
            Column += other.Column;
        } else {
            Column = other.Column;
        }
        return *this;
    }

    inline TSourcePos operator+(const TSourcePos& other) const {
        return TSourcePos(*this) += other;
    }
};

inline TSourcePos SourceOffset(const TStringBuf& buf) {
    size_t lineStart = 0;
    size_t lineOffset = 0;
    for (size_t i = 0; i < buf.length(); ++i) {
        if (buf[i] == '\n') {
            lineStart = i + 1;
            ++lineOffset;
        }
    }
    return TSourcePos(lineOffset, buf.length() - lineStart);
}

template <typename T>
inline TSourcePos SourceOffset(const T& begin, const T& end) {
    T lineStart = begin;
    size_t lineOffset = 0;
    for (T p = begin; p != end; ++p) {
        if (*p == '\n') {
            lineStart = p + 1;
            ++lineOffset;
        }
    }
    return TSourcePos(lineOffset, end - lineStart);
}

struct TSourceLocation {
    TStringBuf Name;
    TSourcePos Pos;

    explicit TSourceLocation()
        : Name("<inline>")
        , Pos()
    {
    }

    explicit TSourceLocation(const TStringBuf& name)
        : Name(name)
        , Pos()
    {
    }

    explicit TSourceLocation(const TSourcePos& pos)
        : Name("<inline>")
        , Pos(pos)
    {
    }

    explicit TSourceLocation(const TStringBuf& name, const TSourcePos& pos)
        : Name(name)
        , Pos(pos)
    {
    }
};

} // NReMorph

Y_DECLARE_OUT_SPEC(inline, NReMorph::TSourcePos, output, pos) {
    output << (pos.Line + 1) << ":" << (pos.Column + 1);
}

Y_DECLARE_OUT_SPEC(inline, NReMorph::TSourceLocation, output, location) {
    output << location.Name << ":" << location.Pos;
}
