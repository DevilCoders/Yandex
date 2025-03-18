#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/strbuf.h>

namespace NInPlaceProto {

    // for passing meta information better than TStringBuf/TArrayRef<const char>
    template <typename TProtoMessage>
    class TSerialized {
    private:
        TArrayRef<const char> Region;

    public:
        TSerialized() noexcept {
        }
        explicit TSerialized(TArrayRef<const char> region) noexcept
            : Region(region)
        {
        }
        const ui8* Data() const noexcept {
            return (const ui8*)Region.data();
        }
        size_t Size() const noexcept {
            return Region.size();
        }
        const TArrayRef<const char>& GetDataRegion() const noexcept {
            return Region;
        }

        explicit operator bool() const noexcept {
            return (bool)Region;
        }
    };

    template <typename TProtoMessage>
    static inline TSerialized<TProtoMessage> AsSerialized(TStringBuf data) {
        return TSerialized<TProtoMessage>(TArrayRef<const char>(data.data(), data.length()));
    }
    template <typename TProtoMessage>
    static inline TSerialized<TProtoMessage> AsSerialized(const char* start, size_t length) {
        return TSerialized<TProtoMessage>(TArrayRef<const char>(start, length));
    }

} // namespace NInPlaceProto
