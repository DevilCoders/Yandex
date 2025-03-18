#pragma once

#include <library/cpp/logger/log.h>
#include <library/cpp/logger/stream.h>

#include <util/datetime/base.h>
#include <util/generic/noncopyable.h>
#include <util/stream/output.h>
#include <util/stream/tempbuf.h>
#include <util/string/cast.h>
#include <util/system/fasttime.h>
#include <util/system/file.h>

class TLogOutput: TNonCopyable {
private:
    mutable TTempBufOutput Buffer;
    mutable bool Padding;

    class TOutput: public IOutputStream {
    public:
        TFileHandle Handle;

        TOutput() noexcept
            : Handle(GetStdErr())
        {
        }

        ~TOutput() override {
            Handle.Release();
        }

    private:
        static FHANDLE GetStdErr() noexcept {
#ifdef _MSC_VER
            return GetStdHandle(STD_ERROR_HANDLE);
#else
            return STDERR_FILENO;
#endif
        }

    private:
        void DoWrite(const void* buf, size_t len) noexcept override {
            Handle.Write(buf, len);
        }

        void DoFlush() noexcept override {
            Handle.Flush();
        }
    };

    static TOutput& Output() noexcept {
        static TOutput output;
        return output;
    }

public:
    TLogOutput(bool padding = true) noexcept
        : Padding(padding)
    {
        if (Padding && Output().Handle.IsOpen()) try {
            struct tm tm;
            *this << Strftime("%Y-%m-%d %H:%M:%S ", TInstant::MicroSeconds(InterpolatedMicroSeconds()).LocalTime(&tm));
        } catch (...) {}
    }

    ~TLogOutput() {
        if (Output().Handle.IsOpen()) try {
            if (Padding) {
                *this << '\n';
            }
            Output().Write(Buffer.Data(), Buffer.Filled());
        } catch (...) {}
    }

    operator IOutputStream&() const noexcept {
        return Buffer;
    }

    static void SetOutput(FHANDLE output) noexcept {
        TFileHandle handle(output);
        Output().Handle.Swap(handle);
        handle.Release();
    }
};

// TIntMapper

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/typetraits.h>
#include <util/system/rwlock.h>

struct TIntMapperThreadsafeOps {
    typedef TRWMutex TLock;
    typedef TReadGuardOps<TLock> TLockReadOps;
    typedef TWriteGuardOps<TLock> TLockWriteOps;
};

template <class T, class I, class TOps = TIntMapperThreadsafeOps>
class TIntMapper: private THashMap<I, T>, public TNonCopyable {
private:
    typename TOps::TLock Lock;

    static_assert(std::is_integral<I>::value, "STATIC_ASSERT_MapResultMustBeIntegral");

    typedef THashMap<I, T> TBase;

    enum {
        MaxCollisions = 32
    };

    THash<T> Hash;
    TEqualTo<T> EqualTo;

    typename TBase::const_iterator find(typename TBase::value_type value) const {
        typename TBase::const_iterator ret = TBase::end();
        unsigned attempt = MaxCollisions;

        TGuard<typename TOps::TLock, typename TOps::TLockReadOps> guard(Lock);

        for (ret = TBase::find(value.first); ret != TBase::end() && !EqualTo(ret->second, value.second) && --attempt; ret = TBase::find(value.first)) {
            const_cast<typename TBase::key_type&>(value.first) = IntHash(value.first);
        }

        if (!attempt) {
            ythrow yexception() << "failed to map"sv;
        }

        return ret;
    }

public:
    I operator[](typename TTypeTraits<T>::TFuncParam t) {
        typename TBase::value_type value(static_cast<const typename TBase::key_type>(Hash(t)), t);
        typename TBase::const_iterator ret = find(value);

        if (ret != TBase::end()) {
            return ret->first;
        }

        unsigned attempt = MaxCollisions;

        TGuard<typename TOps::TLock, typename TOps::TLockWriteOps> guard(Lock);

        for (std::pair<typename TBase::iterator, bool> ret = TBase::insert(value); !ret.second && !EqualTo(ret.first->second, t) && --attempt; ret = TBase::insert(value)) {
            const_cast<typename TBase::key_type&>(value.first) = IntHash(value.first);
        }

        if (!attempt) {
            ythrow yexception() << "failed to map"sv;
        }

        return value.first;
    }

    TMaybe<T> MappedValue(I i) const {
        TGuard<typename TOps::TLock, typename TOps::TLockReadOps> guard(Lock);

        typename TBase::const_iterator ret = TBase::find(i);

        if (ret != TBase::end()) {
            return TMaybe<T>(ret->second);
        }

        return TMaybe<T>();
    }
};

// TPollerCoockie

struct TPollerCookie {
    void* GetCookie() const noexcept {
        return const_cast<void*>(reinterpret_cast<const void*>(this));
    }

    virtual ~TPollerCookie() {
    }
};

// TCopyableIntrusiveListItem

#include <util/generic/intrlist.h>

template <class T>
struct TCopyableIntrusiveListItem: TIntrusiveListItem<T> {
    TCopyableIntrusiveListItem() noexcept {
    }

    TCopyableIntrusiveListItem(const TCopyableIntrusiveListItem& right) noexcept
        : TIntrusiveListItem<T>()
    {
        Y_VERIFY(right.Empty(), "Cant copy linked list item");
    }

    ~TCopyableIntrusiveListItem() {
    }
};

// LoadMapFromStream

#include <util/stream/input.h>
#include <util/string/strip.h>

template <>
inline const TString FromString<const TString>(const TString& s) {
    return s;
}

template <class T>
void LoadMapFromStream(T& map, IInputStream& in) {
    TString line;

    while (in.ReadLine(line)) {
        TString::const_iterator eq = line.end();
        TString::const_iterator it = line.begin();

        bool sQuoted = false;
        bool dQuoted = false;

        bool isEmpty = true;

        for (it = line.begin(); it != line.end(); isEmpty = IsAsciiSpace(*it++) && isEmpty) {
            if (*it == '\'' && !dQuoted) {
                sQuoted = !sQuoted;
            } else if (*it == '"' && !sQuoted) {
                dQuoted = !dQuoted;
            } else if (*it == '#' && !sQuoted && !dQuoted) {
                break;
            } else if (*it == '=' && !sQuoted && !dQuoted) {
                if (eq == line.end()) {
                    eq = it;
                } else {
                    ythrow yexception() << '"' << line << "\": syntax error";
                }
            }
        }

        if (isEmpty) {
            continue;
        }

        if (eq == line.end() || sQuoted || dQuoted) {
            ythrow yexception() << '"' << line << "\": syntax error";
        }

        std::pair<TString::const_iterator, TString::const_iterator> keyRange = std::make_pair(line.begin(), eq);
        std::pair<TString::const_iterator, TString::const_iterator> valRange = std::make_pair(eq + 1, it);

        StripRange(keyRange.first, keyRange.second);
        StripRange(valRange.first, valRange.second);

        TString keyStr;
        TString valStr;

        sQuoted = false;
        dQuoted = false;

        for (it = keyRange.first; it != keyRange.second; ++it) {
            if (*it == '\'' && !dQuoted) {
                sQuoted = !sQuoted;
            } else if (*it == '"' && !sQuoted) {
                dQuoted = !dQuoted;
            } else {
                keyStr.append(*it);
            }
        }

        sQuoted = false;
        dQuoted = false;

        for (it = valRange.first; it != valRange.second; ++it) {
            if (*it == '\'' && !dQuoted) {
                sQuoted = !sQuoted;
            } else if (*it == '"' && !sQuoted) {
                dQuoted = !dQuoted;
            } else {
                valStr.append(*it);
            }
        }

        typename T::value_type value(FromString<const typename T::value_type::first_type>(keyStr), FromString<typename T::value_type::second_type>(valStr));

        std::pair<typename T::iterator, bool> ret = map.insert(value);

        if (!ret.second) {
            DoSwap(ret.first->second, value.second);
        }
    }
}
