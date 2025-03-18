#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

namespace NFuse {

class TArgParser {
public:
    TArgParser()
    {
    }

    TArgParser(TStringBuf arg)
        : buf(arg)
    {
    }

    // Reads trivial struct or primitive
    template<class T>
    const T& Read() {
        static_assert(std::is_trivial_v<T>);
        Y_ENSURE(0 == reinterpret_cast<uintptr_t>(buf.data()) % alignof(T),
                 "unaligned struct data");
        Y_ENSURE(buf.size() >= sizeof(T));
        const void* data = buf.data();
        buf.remove_prefix(sizeof(T));
        return *static_cast<const T*>(data);
    }

    // Reads zero-terminated string
    TStringBuf ReadStr() {
        size_t end = buf.find('\0');
        Y_ENSURE(end != TStringBuf::npos, "no null-terminator found");
        TStringBuf res = buf.substr(0, end);
        buf.remove_prefix(end+1);
        return res;
    }

    TStringBuf Raw() const {
        return buf;
    }

private:
    TStringBuf buf;
};

} // namespace NFuse
