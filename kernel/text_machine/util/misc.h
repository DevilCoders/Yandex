#pragma once

#include <kernel/text_machine/interface/hit.h>
#include <kernel/text_machine/interface/query.h>

namespace NTextMachine {

// "Deep" equality for
// TM interface structures, useful for tests
//
inline bool operator==(const TStream& lhs, const TStream& rhs) {
    bool res = true;
    res = res && (lhs.AnnotationCount == rhs.AnnotationCount);
    res = res && (lhs.WordCount == rhs.WordCount);
    res = res && (lhs.MaxValue == rhs.MaxValue);
    res = res && (lhs.Type == rhs.Type);
    return res;
}

inline bool operator!=(const TStream& lhs, const TStream& rhs) {
    return !(lhs == rhs);
}

inline bool operator==(const TAnnotation& lhs, const TAnnotation& rhs) {
    Y_ASSERT(lhs.Stream && rhs.Stream);
    if (!lhs.Stream || !rhs.Stream)
        return false;
    bool res = true;
    res = res && (lhs.Value == rhs.Value);
    res = res && (lhs.BreakNumber == rhs.BreakNumber);
    res = res && (lhs.FirstWordPos == rhs.FirstWordPos);
    res = res && (lhs.Length == rhs.Length);
    res = res && (*(lhs.Stream) == *(rhs.Stream));
    res = res && (lhs.StreamIndex == rhs.StreamIndex);
    return res;
}

inline bool operator!=(const TAnnotation& lhs, const TAnnotation& rhs) {
    return !(lhs == rhs);
}

inline bool operator==(const TPosition& lhs, const TPosition& rhs) {
    Y_ASSERT(lhs.Annotation && rhs.Annotation);
    if (!lhs.Annotation || !rhs.Annotation)
        return false;
    return lhs.LeftWordPos == rhs.LeftWordPos
        && lhs.RightWordPos == rhs.RightWordPos
        && *(lhs.Annotation) == *(rhs.Annotation);
}

inline bool operator==(const THitWord& lhs, const THitWord& rhs) {
    return lhs.FormId == rhs.FormId && lhs.QueryId == rhs.QueryId && lhs.WordId == rhs.WordId;
}

inline bool operator==(const THit& lhs, const THit& rhs) {
    return lhs.Weight == rhs.Weight && lhs.Position == rhs.Position && lhs.Word == rhs.Word;
}

inline bool operator!=(const THit& lhs, const THit& rhs) {
    return !(lhs == rhs);
}

inline bool operator==(const TBlockHit& lhs, const TBlockHit& rhs) {
    return lhs.Weight == rhs.Weight &&
        lhs.Position == rhs.Position &&
        lhs.BlockId == rhs.BlockId &&
        lhs.LemmaId == rhs.LemmaId &&
        lhs.Precision == rhs.Precision;
}

inline bool operator!=(const TBlockHit& lhs, const TBlockHit& rhs) {
    return !(lhs == rhs);
}

// Debug I/O
//
inline IOutputStream& operator<<(IOutputStream& out, const TStream& rhs) {
    out << "Type: " << rhs.Type
        << ", " << "AnnCount: " << rhs.AnnotationCount
        << ", " << "WordCount: " << rhs.WordCount
        << ", " << "MaxValue: " << rhs.MaxValue;
    return out;
}

inline IOutputStream& operator<<(IOutputStream& out, const TAnnotation& rhs) {
    Y_ASSERT(rhs.Stream);
    out << "StreamType: " << rhs.Stream->Type << "(" << rhs.StreamIndex << ")"
        << ", " << "BreakNumber: " << rhs.BreakNumber
        << ", " << "FirstWordPos: " << rhs.FirstWordPos
        << ", " << "Length: " << rhs.Length
        << ", " << "Value: " << rhs.Value;
    return out;
}

inline IOutputStream& operator<<(IOutputStream& out, const TPosition& rhs) {
    Y_ASSERT(rhs.Annotation);
    out << "Annotation {" << *rhs.Annotation << "}"
        << ", " << "LeftWordPos: " << rhs.LeftWordPos
        << ", " << "RightWordPos: " << rhs.RightWordPos;
    return out;
}

inline IOutputStream& operator<<(IOutputStream& out, const THit& rhs) {
    out << "QueryId: " << rhs.Word.QueryId
        << ", " << "WordId: " << rhs.Word.WordId
        << ", " << "FormId: " << rhs.Word.FormId
        << ", " << "Weight: " << rhs.Weight
        << ", " << "Position {" << rhs.Position << "}";
    return out;
}

inline IOutputStream& operator<<(IOutputStream& out, const TBlockHit& rhs) {
    out << "BlockId: " << rhs.BlockId
        << ", " << "LemmaId: " << rhs.LemmaId
        << ", " << "Precision: " << rhs.Precision
        << ", " << "Weight: " << rhs.Weight
        << ", " << "Position {" << rhs.Position << "}";
    return out;
}

inline IOutputStream& operator<<(IOutputStream& out, const TQueryWordMatch& rhs) {
    out << "Idf: " << rhs.IdfsByType[TRevFreq::Default]
        << ", " << "Type: " << rhs.MatchType
        << ", " << "Precision: " << rhs.MatchPrecision
        << ", " << "BlockId: " << rhs.MatchedBlockId
        << ", " << "Weight: " << rhs.Weight;
    return out;
}

inline IOutputStream& operator<<(IOutputStream& out, const TQueryWord& rhs) {
    out << "Idf: " << rhs.IdfsByType[TRevFreq::Default];
    for (const TQueryWordMatch* match : rhs.Forms) {
        out << ", " << "Match {" << *match << "}";
    }
    return out;
}

inline IOutputStream& operator<<(IOutputStream& out, const TQueryType& rhs) {
    out << "RegionClass: " << rhs.RegionClass;
    return out;
}

inline IOutputStream& operator<<(IOutputStream& out, const TQuery& rhs) {
    out << "Type: " << rhs.ExpansionType;
    for (size_t i = 0; i != rhs.Values.size(); ++i) {
        out << ", " << "Value {Type: {" << rhs.Values[i].Type << "}"
            << ", Value: " << rhs.Values[i].Value << "}";
    }
    for (size_t i = 0; i != rhs.Words.size(); ++i) {
        out << ", " << "Word {" << rhs.Words[i] << "}";

        if (i < rhs.Cohesion.size() && rhs.Cohesion[i] > 0.0) {
            out << ", " << "Cohesion: " << rhs.Cohesion[i];
        }
    }
    return out;
}

}
