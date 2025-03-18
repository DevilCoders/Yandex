#pragma once

#include "idxr_base.h"

#include <library/cpp/codecs/pfor_codec.h>

#include <util/generic/array_ref.h>

namespace NIndexedRegion {
    using namespace NCodecs;

    template <typename TIdx>
    struct TGetFromRegion<TIdx, TArrayRef<const TIdx>> {
        TIdx operator()(const TArrayRef<const TIdx>& reg, size_t off) const {
            TIdx res;
            memcpy(&res, reg.data() + off, sizeof(TIdx));
            return res;
        }
    };

    template <typename TIdx>
    class TSmallIndexedRegion: public TIndexedBase<TArrayRef<const TIdx>, TIdx> {
        using TIndexRegion = TArrayRef<const TIdx>;
        using TBase = TIndexedBase<TIndexRegion, TIdx>;

    public:
        using TIndexElementType = TIdx;

    private:
        TCodecPtr IndexCodec;
        TCodecPtr DataCodec;

    public:
        static TIndexRegion AsIndexRegion(TStringBuf b) {
            Y_ASSERT(!(b.size() % sizeof(TIdx)));
            return TIndexRegion{(const TIdx*)b.begin(), (const TIdx*)b.end()};
        }

        static TIndexRegion AsIndexRegion(const TVector<TIdx>& idx) {
            return TIndexRegion{idx.data(), idx.size()};
        }

        static TStringBuf AsDataRegion(TIndexRegion idx) {
            return TStringBuf{(const char*)idx.begin(), (const char*)idx.end()};
        }

    public:
        // specify TTrivialCodec if you want the region to own memory and do not want the compression
        explicit TSmallIndexedRegion(const TCodecPtr& idxc = nullptr,
                                     const TCodecPtr& bufc = nullptr) {
            SetIndexCodec(idxc);
            SetDataCodec(bufc);
        }

        TCodecPtr GetIndexCodec() const {
            return IndexCodec;
        }

        TCodecPtr GetDataCodec() const {
            return DataCodec;
        }

        void DoCommit() override {
            TBase::DataBuffer.ShrinkToFit();
            TBase::DataRegion = TBase::AsDataRegion(TBase::DataBuffer);
            TBase::IndexRegion = AsIndexRegion(TBase::IndexBuffer);
        }

        void Encode(TBuffer& b) const override {
            if (TBase::Empty()) {
                return;
            }

            TBuffer buff;
            TStringBuf indexRegionRaw = AsDataRegion(TBase::IndexRegion);

            if (IndexCodec.Get()) {
                IndexCodec->Encode(indexRegionRaw, buff);
            } else {
                buff.Append(indexRegionRaw.data(), indexRegionRaw.size());
            }

            {
                NBitIO::TBitOutputVector<TBuffer> bout(&b);
                bout.WriteWords<15>(buff.Size());
            }

            b.Append(buff.data(), buff.size());

            if (DataCodec.Get()) {
                buff.Clear();
                DataCodec->Encode(TBase::DataRegion, buff);
                b.Append(buff.data(), buff.size());
            } else {
                b.Append(TBase::DataRegion.data(), TBase::DataRegion.size());
            }
        }

        void CopyCodecsTo(TSmallIndexedRegion<TIdx>& r) const {
            r.ClearNoReset();

            r.IndexCodec = IndexCodec;
            r.DataCodec = DataCodec;
        }

        void CopyTo(TSmallIndexedRegion<TIdx>& r) const {
            CopyCodecsTo(r);

            if (TBase::DataRegion) {
                if (TBase::DataBuffer && TBase::DataRegion.begin() == TBase::DataBuffer.Begin()) {
                    r.DataBuffer = TBase::DataBuffer;
                    r.DataRegion = TBase::AsDataRegion(r.DataBuffer);
                } else {
                    r.DataRegion = TBase::DataRegion;
                }
            } else {
                r.DataRegion.Clear();
            }

            if (TBase::IndexRegion) {
                if (TBase::IndexBuffer && (void*)TBase::IndexRegion.begin() == (void*)TBase::IndexBuffer.begin()) {
                    r.IndexBuffer = TBase::IndexBuffer;
                    r.IndexRegion = AsIndexRegion(r.IndexBuffer);
                } else if (TBase::DataBuffer && (void*)TBase::IndexRegion.begin() == (void*)TBase::DataBuffer.Begin()) {
                    r.DataBuffer = TBase::DataBuffer;
                    r.IndexRegion = AsIndexRegion(TBase::AsDataRegion(r.DataBuffer));
                } else {
                    r.IndexRegion = TBase::IndexRegion;
                }
            } else {
                r.IndexRegion = TIndexRegion();
            }
        }

        void Decode(TStringBuf r) override {
            TBase::Clear();

            if (r.empty()) {
                return;
            }

            NBitIO::TBitInput bin(r);
            ui64 idxlen = 0;

            if (Y_UNLIKELY(!bin.ReadWords<15>(idxlen))) {
                ythrow TInvalidDataException();
            }

            r.Skip(bin.GetOffset());

            if (Y_UNLIKELY(idxlen > r.size())) {
                ythrow TInvalidDataException();
            }

            TStringBuf idxr = r.SubStr(0, idxlen);
            r.Skip(idxlen);

            if (IndexCodec) {
                TBase::DataBuffer.Reserve(idxlen * 64);
                IndexCodec->Decode(idxr, TBase::DataBuffer);

                TBase::IndexRegion = AsIndexRegion(TBase::AsDataRegion(TBase::DataBuffer));

                if (DataCodec) {
                    TBase::IndexBuffer.assign(TBase::IndexRegion.begin(),
                                              TBase::IndexRegion.end());
                    TBase::IndexRegion = AsIndexRegion(TBase::IndexBuffer);
                }
            } else {
                TBase::IndexRegion = AsIndexRegion(idxr);
            }

            if (DataCodec.Get()) {
                TBase::DataBuffer.Clear();
                TBase::DataBuffer.Reserve(
                    DataCodec->Traits().ApproximateSizeOnDecode(r.size()));
                DataCodec->Decode(r, TBase::DataBuffer);
                TBase::DataRegion = TBase::AsDataRegion(TBase::DataBuffer);
            } else {
                TBase::DataRegion = r;
            }
        }

        void SetIndexCodec(const TCodecPtr& idxc) {
            Y_VERIFY(!idxc || !(sizeof(TIdx) % idxc->Traits().SizeOfInputElement), " ne-ne-ne");
            IndexCodec = idxc;
        }

        void SetDataCodec(const TCodecPtr& dc) {
            DataCodec = dc;
        }
    };

    class TAdaptiveSmallRegion {
        TSmallIndexedRegion<ui32> Region32;
        TSmallIndexedRegion<ui64> Region64;
        bool Use64 = false;

    public:
        typedef NPrivate::TConstIterator<TAdaptiveSmallRegion> TConstIterator;

        static bool Use64BitLengths(const ICodec* c) {
            return !!c && c->Traits().SizeOfInputElement == 8;
        }

        TAdaptiveSmallRegion() = default;

        TAdaptiveSmallRegion(const TCodecPtr& len, const TCodecPtr& data = nullptr) {
            Init(len, data);
        }

        void Init(const TCodecPtr& len, const TCodecPtr& data = nullptr) {
            Use64 = Use64BitLengths(len.Get());
            if (Use64) {
                Region64.SetIndexCodec(len);
                Region64.SetDataCodec(data);
            } else {
                Region32.SetIndexCodec(len);
                Region32.SetDataCodec(data);
            }
        }

        void PushBack(TStringBuf r) {
            if (Use64) {
                Region64.PushBack(r);
            } else {
                Region32.PushBack(r);
            }
        }

        void Commit() {
            if (Use64) {
                Region64.Commit();
            } else {
                Region32.Commit();
            }
        }

        void Decode(TStringBuf r) {
            if (Use64) {
                Region64.Decode(r);
            } else {
                Region32.Decode(r);
            }
        }

        void Encode(TBuffer& b) const {
            if (Use64) {
                Region64.Encode(b);
            } else {
                Region32.Encode(b);
            }
        }

        size_t Size() const {
            return Use64 ? Region64.Size() : Region32.Size();
        }

        TConstIterator LowerBound(TStringBuf r) const {
            if (Use64) {
                TSmallIndexedRegion<ui64>::TConstIterator it = ::LowerBound(
                    Region64.Begin(), Region64.End(), r);
                return TConstIterator(this, it.Offset);
            } else {
                TSmallIndexedRegion<ui32>::TConstIterator it = ::LowerBound(
                    Region32.Begin(), Region32.End(), r);
                return TConstIterator(this, it.Offset);
            }
        }

        TStringBuf Get(size_t idx) const {
            return Use64 ? Region64.Get(idx) : Region32.Get(idx);
        }

        void CopyTo(TAdaptiveSmallRegion& other) const {
            other.Use64 = Use64;
            if (Use64) {
                Region64.CopyTo(other.Region64);
            } else {
                Region32.CopyTo(other.Region32);
            }
        }

        void CopyCodecsTo(TAdaptiveSmallRegion& other) const {
            other.Use64 = Use64;
            if (Use64) {
                Region64.CopyCodecsTo(other.Region64);
            } else {
                Region32.CopyCodecsTo(other.Region32);
            }
        }

        void Clear() {
            if (Use64) {
                Region64.ClearNoReset();
            } else {
                Region32.ClearNoReset();
            }
        }

        TStringBuf operator[](size_t idx) const {
            return Get(idx);
        }

        TConstIterator Begin() const {
            return TConstIterator(this, 0);
        }

        TConstIterator End() const {
            return TConstIterator(this, Size());
        }
    };

}
