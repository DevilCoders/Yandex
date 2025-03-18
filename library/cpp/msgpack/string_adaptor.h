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
struct convert<TString> {
    msgpack::object const& operator()(msgpack::object const& o, TString& v) const {
        switch (o.type) {
        case msgpack::type::BIN:
            v.assign(o.via.bin.ptr, o.via.bin.size);
            break;
        case msgpack::type::STR:
            v.assign(o.via.str.ptr, o.via.str.size);
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
