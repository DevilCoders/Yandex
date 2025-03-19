#pragma once

#include <library/cpp/vec4/vec4.h>

#include <util/system/types.h>
#include <util/stream/output.h>
#include <util/generic/array_ref.h>

#include <array>

class IInputStream;
class IOutputStream;

namespace NIndexAnn {
    class THit {
        friend class THitMask;
    public:
        using TDocId = ui32;
        using TBreak = ui16;
        using TRegion = ui32;
        using TStream = ui16;
        using TValue = ui32;

        THit()
            : M128_()
        {}

        THit(const THit& other)
            : M128_(other.M128_)
        {}

        THit(TDocId docId, TBreak breuk, TRegion region, TStream stream, TValue value)
            : DocId_(docId)
            , Break_(breuk)
            , Region_(region)
            , Stream_(stream)
            , Value_(value)
        {}

        THit& operator=(const THit& other) {
            M128_ = other.M128_;
            return *this;
        }

        TDocId DocId() const {
            return DocId_;
        }

        void SetDocId(ui32 docId) {
            DocId_ = docId;
        }

        TBreak Break() const {
            return Break_;
        }

        void SetBreak(ui16 breuk) {
            Break_ = breuk;
        }

        TRegion Region() const {
            return Region_;
        }

        void SetRegion(ui32 region) {
            Region_ = region;
        }

        TStream Stream() const {
            return Stream_;
        }

        void SetStream(ui16 stream) {
            Stream_ = stream;
        }

        TValue Value() const {
            return Value_;
        }

        void SetValue(ui32 value) {
            Value_ = value;
        }

        friend bool operator==(const THit& l, const THit& r) {
            return l.M128_ == r.M128_;
        }

        friend bool operator!=(const THit& l, const THit& r) {
            return !(l == r);
        }

        friend bool operator<(const THit& l, const THit& r) {
            return std::array<ui32, 5>{ l.DocId_, l.Break_, l.Region_, l.Stream_, l.Value_ } <
                   std::array<ui32, 5>{ r.DocId_, r.Break_, r.Region_, r.Stream_, r.Value_ };
        }

    private:
        union {
#pragma pack(push)
#pragma pack(1)
            struct {
                TDocId DocId_;
                TBreak Break_;
                TRegion Region_;
                TStream Stream_;
                TValue Value_;
            };
#pragma pack(pop)
            TVec4u M128_;
        };
        static_assert(sizeof(TDocId) + sizeof(TBreak) + sizeof(TRegion) + sizeof(TStream) + sizeof(TValue) == sizeof(TVec4u), "fields don't sum to TVec4u");
    };

    IOutputStream& operator<<(IOutputStream& out, const THit& hit);

    static_assert(sizeof(THit) == 16, "Expected sizeof(THit) == 16.");

    class THitMask : public THit {
    public:
        THitMask(THit::TDocId docId = -1,
                 THit::TBreak breuk = -1,
                 THit::TRegion region = -1,
                 THit::TStream stream = -1)
        {
            SetDocId(docId);
            SetBreak(breuk);
            SetRegion(region);
            SetStream(stream);
        }

        void SetDocId(THit::TDocId docId) {
            if (docId == Max<THit::TDocId>()) {
                THit::SetDocId(0);
                Mask_.SetDocId(0);
            } else {
                THit::SetDocId(docId);
                Mask_.SetDocId(Max<THit::TDocId>());
            }
        }

        void SetBreak(THit::TBreak breuk) {
            if (breuk == Max<THit::TBreak>()) {
                THit::SetBreak(0);
                Mask_.SetBreak(0);
            } else {
                THit::SetBreak(breuk);
                Mask_.SetBreak(Max<THit::TBreak>());
            }
        }

        void SetRegion(THit::TRegion region) {
            if (region == Max<THit::TRegion>()) {
                THit::SetRegion(0);
                Mask_.SetRegion(0);
            } else {
                THit::SetRegion(region);
                Mask_.SetRegion(Max<THit::TRegion>());
            }
        }

        void SetStream(THit::TStream stream) {
            if (stream == Max<THit::TStream>()) {
                THit::SetStream(0);
                Mask_.SetStream(0);
            } else {
                THit::SetStream(stream);
                Mask_.SetStream(Max<THit::TStream>());
            }
        }

        bool HasDoc() const {
            return Mask_.DocId();
        }

        bool HasBreak() const {
            return Mask_.Break();
        }

        bool HasRegion() const {
            return Mask_.Region();
        }

        bool HasStream() const {
            return Mask_.Stream();
        }

        bool Matches(const THit& hit) const {
            return (hit.M128_ & Mask_.M128_) == M128_;
        }

        bool IsNull() const {
            return Mask_.M128_ == TVec4u(0, 0, 0, 0);
        }

    private:
        THit Mask_;
     };

} // NIndexAnn
