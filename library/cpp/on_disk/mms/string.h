#pragma once

#include "impl/tags.h"
#include "writer.h"

#include <contrib/libs/mms/impl/container.h>
#include <contrib/libs/mms/impl/fwd.h>
#include <contrib/libs/mms/version.h>

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/str_stl.h>
#include <util/stream/output.h>

namespace NMms {
    template <class P>
    class TStringType;

    template <>
    class TStringType<TMmapped>: public mms::impl::Container, public TStringBase<TStringType<TMmapped>, char> {
    public:
        using MmappedType = TStringType<TMmapped>;

        using TContainerBase = mms::impl::Container;
        using TStringBase = TStringBase<TStringType<TMmapped>, char>;

        TStringType()
            : TContainerBase(Zero(), 0)
        {
        }
        TStringType(const TStringBuf& str)
            : TContainerBase(str.data(), str.size())
        {
            //Y_ENSURE(str.data()[str.size()] == '\0', "the string has no trailing zero"); // TODO: currently fails =(
        }
        TStringType(const TString& str)
            : TContainerBase(str.data(), str.size())
        {
        }
        TStringType(const char* str)
            : TContainerBase(str, TStringBase::StrLen(str))
        {
        }
        TStringType(const char* str, size_t size)
            : TContainerBase(str, size)
        {
            Y_ENSURE(str[size] == '\0', "the string has no trailing zero");
        }

        // required by TStringBase
        const char* data() const noexcept {
            return TContainerBase::ptr<char>();
        }

        const char* c_str() const noexcept {
            return data();
        }

        // required by TStringBase
        size_t length() const noexcept {
            return TContainerBase::size();
        }

        // Resolve the ambiguity: the two size() and empty() methods return exactly the same.
        // Container::size() stores length of string (not including stored trailing zero)
        using TStringBase::empty;
        using TStringBase::size;

        static mms::FormatVersion formatVersion(mms::Versions& /*vs*/) {
            return mms::Versions::hash("string");
        }

    private:
        static const char* Zero() noexcept {
            static const char z = '\0';
            return &z;
        }
    };

    template <>
    class TStringType<TStandalone>: public TString {
    public:
        using MmappedType = TStringType<TMmapped>;

        using TString::TString;

        TStringType(const MmappedType& s)
            : TString(s)
        {
        }
        TStringType(const TString& s) noexcept : TString(s) {}
        TStringType(TString&& s) noexcept : TString(std::move(s)) {}

        template <class TWriter>
        size_t writeData(TWriter& w) const {
            mms::impl::align(w);
            const size_t res = w.pos();
            w.write(c_str(), size() + 1); // +1 for trailing zero
            return res;
        }

        template <class TWriter>
        size_t writeField(TWriter& w, size_t pos) const {
            return mms::impl::writeRef(w, pos, size());
        }
    };

    // size of string is just size and offset
    // base empty class optimization must work in this case
    static_assert(sizeof(TStringType<TMmapped>) == 2 * sizeof(size_t), "NMms::TStringType size mismatch");

}

// Output
Y_DECLARE_OUT_SPEC(inline, NMms::TStringType<NMms::TStandalone>, out, s) {
    out.Write(s.data(), s.size());
}
Y_DECLARE_OUT_SPEC(inline, NMms::TStringType<NMms::TMmapped>, out, s) {
    out.Write(s.data(), s.size());
}

// Hashes
template <class P>
struct THash<NMms::TStringType<P>> {
    inline size_t operator()(const NMms::TStringType<P>& s) const {
        return ComputeHash(TStringBuf{s});
    }
};
