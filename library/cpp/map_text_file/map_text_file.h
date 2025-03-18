#pragma once

#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/memory/blob.h>
#include <util/string/split.h>

/**
fast, malloc-free iteration over memory-mapped text file:

for (TStringBuf line: TMapTextFile("filename.txt")) {
    //parse line to 3 fields, without memory reallocation:
    TStringBuf query, countStr;
    int field3;
    Split(line, '\t', query, countStr, field3);
    ...
}

TMapTextFile::LineCount can be used to determine number of line-records

TMapTextFile txt("filename.txt")
TVector<TStringBuf> lines(txt.LineCount()); //references to memory-mapped data
size_t i = 0;
for (TStringBuf line: txt)
    lines.at(i++) = line;

//when you do not care about reallocations:
TVector<TString>(txt.begin(), txt.end())

*/

struct TIterTextLines: public std::iterator<std::forward_iterator_tag, TStringBuf> {
    TIterTextLines(const char* textPtr, size_t n) {
        EndPtr = textPtr + n;
        Finished = (n == 0);
        LineStart = textPtr;
        NextInternalFirst(); //setup LineEnd
    }

    //default iterator can be used as 'end'
    TIterTextLines()
        : LineStart(nullptr)
        , LineEnd(nullptr)
        , LineEndCrLf(nullptr)
        , EndPtr(nullptr)
        , Finished(true)
    {
    }

    bool Valid() const {
        return !Finished;
    }

    void Next() {
        Y_ENSURE(!Finished, "TIterTextLines::Next after EOF");
        if (LineEnd == EndPtr) {
            //this branch for files with no EOL at last line
            Finished = true;
            return;
        }
        LineStart = LineEnd + 1;
        if (LineStart == EndPtr) {
            Finished = true;
            return;
        }
        NextInternalFirst();
    }

    bool operator==(const TIterTextLines& r) const {
        return (r.Finished == Finished) && (Finished || r.LineStart == LineStart);
    }

    bool operator!=(const TIterTextLines& r) const {
        return !(*this == r);
    }

    void operator++() {
        Next();
    }

    TStringBuf operator*() {
        Y_ENSURE(!Finished, "reading TIterTextLines after EOF");
        return TStringBuf(LineStart, LineEndCrLf);
    }

private:
    void NextInternal() {
        LineEnd = (const char*)memchr(LineStart, '\n', EndPtr - LineStart);
        if (!LineEnd) {
            //this branch for files with no EOL at last line
            LineEnd = EndPtr;
            LineEndCrLf = EndPtr;
        } else {
            // LineEnd[-1] is valid, because case (LineEnd==(start of memory map)) is handled by NextInternalFirst
            if (LineEnd[-1] == '\r') {
                LineEndCrLf = LineEnd - 1;
            } else {
                LineEndCrLf = LineEnd;
            }
        }
    }

    void NextInternalFirst() {
        LineEnd = (const char*)memchr(LineStart, '\n', EndPtr - LineStart);
        if (!LineEnd) {
            //this branch for files with no EOL at last line
            LineEnd = EndPtr;
            LineEndCrLf = EndPtr;
        } else {
            if (LineEnd != LineStart && LineEnd[-1] == '\r') {
                LineEndCrLf = LineEnd - 1;
            } else {
                LineEndCrLf = LineEnd;
            }
        }
    }

private:
    const char* LineStart = nullptr;
    const char* LineEnd = nullptr;
    const char* LineEndCrLf = nullptr;
    const char* EndPtr = nullptr;
    bool Finished = true;
};

class TMapTextFile {
    TBlob Blob;
    TString Fn;

public:
    typedef TIterTextLines iterator;

    TMapTextFile(const TString& fn)
        : Blob(TBlob::FromFile(fn))
        , Fn(fn)
    {
    }

    TIterTextLines begin() const {
        return TIterTextLines((const char*)Blob.Begin(), Blob.Size());
    }
    TIterTextLines end() const {
        return TIterTextLines();
    }

    size_t LineCount() {
        return std::distance(begin(), end());
    }
};
