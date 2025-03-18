#pragma once

#include <util/generic/maybe.h>
#include <util/generic/buffer.h>
#include <util/generic/vector.h>

#include <util/stream/buffer.h>
#include <util/stream/output.h>
#include <util/stream/printf.h>

#include <util/string/cast.h>

inline void AddField(TBufferOutput& stream, const char* name, const char* value) {
    if (value) {
        if (stream.Buffer().Size() > 0)
            stream << '\t';
        stream << name << '=' << value;
    }
}

template <class TName, class TVal>
inline void AddField(TBufferOutput& stream, const TName& name, const TVal& value) {
    if (stream.Buffer().Size() > 0)
        stream << '\t';
    stream << name << '=' << value;
}

template <class TName, class TVal>
inline void AddField(TBufferOutput& stream, const TName& name, const TMaybe<TVal>& value) {
    if (value.Defined()) {
        AddField(stream, name, *value);
    }
}

template <class TVal>
inline void AddField(TBufferOutput& stream, const TVal& value) {
    if (stream.Buffer().Size() > 0)
        stream << '\t';
    stream << value;
}

// TODO - unite AddField & AddCSField

inline void AddCSField(TBufferOutput& stream, const char* name, const char* value, const TStringBuf& sep) {
    if (value) {
        if (stream.Buffer().Size() > 0)
            stream << sep;
        stream << name << '=' << value;
    }
}

template <class TName, class TVal>
inline void AddCSField(TBufferOutput& stream, const TName& name, const TVal& value, const TStringBuf& sep) {
    if (stream.Buffer().Size() > 0)
        stream << sep;
    stream << name << '=' << value;
}

template <class TName, class TVal>
inline void AddCSField(TBufferOutput& stream, const TName& name, const TMaybe<TVal>& value, const TStringBuf& sep) {
    if (value.Defined()) {
        AddCSField(stream, name, *value, sep);
    }
}

template <class TVal>
inline void AddCSField(TBufferOutput& stream, const TVal& value, const TStringBuf& sep) {
    if (stream.Buffer().Size() > 0)
        stream << sep;
    stream << value;
}

template <class T>
void ReadFields(IInputStream& in, TVector<T>& v, char delim = ',') {
    v.clear();
    TString el;
    while (in.ReadTo(el, delim)) {
        v.push_back(FromString<T>(el));
    }
}

template <class T>
void WriteFields(IOutputStream& out, const TVector<T>& v, char delim = ',') {
    if (!v.empty()) {
        typename TVector<T>::const_iterator it = v.begin();
        out << *it;
        for (++it; it != v.end(); ++it) {
            out << delim << *it;
        }
    }
}

template <class T>
void WriteFieldsFmt(IOutputStream& out, const TVector<T>& v, const char* fmt, char delim = ',') {
    if (!v.empty()) {
        typename TVector<T>::const_iterator it = v.begin();
        Printf(out, fmt, *it);
        for (++it; it != v.end(); ++it) {
            out << delim;
            Printf(out, fmt, *it);
        }
    }
}
