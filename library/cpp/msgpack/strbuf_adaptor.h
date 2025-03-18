#pragma once

#include <contrib/libs/msgpack/include/msgpack/versioning.hpp>
#include <contrib/libs/msgpack/include/msgpack/adaptor/adaptor_base.hpp>

#include <util/generic/string.h>
#include <util/generic/strbuf.h>

namespace msgpack {

/// @cond
MSGPACK_API_VERSION_NAMESPACE(v1) {
/// @endcond

namespace adaptor {

template <>
struct convert<TStringBuf> {
    msgpack::object const& operator()(msgpack::object const& o, TStringBuf& v) const {
        switch (o.type) {
        case msgpack::type::BIN:
            v = TStringBuf(o.via.bin.ptr, o.via.bin.size);
            break;
        case msgpack::type::STR:
            v = TStringBuf(o.via.str.ptr, o.via.str.size);
            break;
        default:
            throw msgpack::type_error();
            break;
        }
        return o;
    }
};

} // namespace adaptor

/// @cond
}  // MSGPACK_API_VERSION_NAMESPACE(v1)
/// @endcond

}  // namespace msgpack
