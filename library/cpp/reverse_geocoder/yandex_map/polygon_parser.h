#pragma once

#include <library/cpp/reverse_geocoder/proto/region.pb.h>

#include <util/generic/strbuf.h>
#include <util/generic/noncopyable.h>

namespace NReverseGeocoder {
    namespace NYandexMap {
        class TPolygonParser: public TNonCopyable {
        public:
            enum class EState {
                FirstClosed,
                SecondClosed,
                SecondOpened,
                Delimeter,
                PolygonDelimeter,
                Parsed,
            };

            TPolygonParser(NProto::TRegion& region)
                : State_(EState::FirstClosed)
                , Begin_(nullptr)
                , End_(nullptr)
                , Region_(&region)
            {
            }

            void Parse(const TStringBuf& str);

            void Parse(const char*& ptr, const char* begin, const char* end);

        private:
            void ParseImpl(const char*& ptr);

            EState State_;
            const char* Begin_;
            const char* End_;
            NProto::TRegion* Region_;
            size_t PolyNum{};
        };

        class TMultiPolygonParser: public TNonCopyable {
        public:
            enum class EState {
                Closed,
                Opened,
                Delimeter,
                Parsed,
            };

            TMultiPolygonParser(NProto::TRegion& region)
                : State_(EState::Closed)
                , Begin_(nullptr)
                , End_(nullptr)
                , Region_(&region)
            {
            }

            void Parse(const TStringBuf& str);

        private:
            void ParseImpl(const char*& ptr);

            EState State_;
            const char* Begin_;
            const char* End_;
            NProto::TRegion* Region_;
        };

    }
}
