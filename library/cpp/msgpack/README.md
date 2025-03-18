## Arcadia type adaptors for msgpack

This library contains msgpack adaptors for types from `util/` and `library/`:
- `string_adaptor.h`: TString
- `strbuf_adaptor.h`: TStringBuf

`adaptor.h` is a universal include-all entry point for client code (original
msgpack library exports all adaptors via `<msgpack/type.hpp>`).
