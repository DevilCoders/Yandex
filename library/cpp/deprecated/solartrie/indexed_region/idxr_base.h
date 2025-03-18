#pragma once

#include <library/cpp/containers/paged_vector/paged_vector.h>

#include <library/cpp/deprecated/accessors/accessors.h>

#include <util/generic/bt_exception.h>
#include <util/generic/buffer.h>
#include <util/generic/noncopyable.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NIndexedRegion {
    class TInvalidDataException: public TWithBackTrace<yexception> {};

    namespace NPrivate {
        template <typename TIndexedRegion>
        struct TConstIterator {
            using TSelf = TConstIterator<TIndexedRegion>;
            using TOwner = TIndexedRegion;

            const TOwner* Region;
            ui64 Offset;

            TConstIterator(const TOwner* r = 0, ui64 off = -1)
                : Region(r)
                , Offset(off)
            {
            }

            TStringBuf operator*() const {
                return Region->Get(Offset);
            }

            bool operator==(const TSelf& it) const {
                Y_VERIFY(Region == it.Region, " YOU DO IT WRONG!");
                return Offset == it.Offset;
            }

            bool operator!=(const TSelf& it) const {
                return !(*this == it);
            }

            bool operator<(const TSelf& it) const {
                Y_VERIFY(Region == it.Region, "YOU DO IT WRONG!");
                return Offset < it.Offset;
            }

            bool operator<=(const TSelf& it) const {
                Y_VERIFY(Region == it.Region, "YOU DO IT WRONG!");
                return Offset <= it.Offset;
            }

            bool operator>(const TSelf& it) const {
                return !(*this <= it);
            }

            bool operator>=(const TSelf& it) const {
                return !(*this < it);
            }

            ptrdiff_t operator-(const TSelf& it) const {
                Y_VERIFY(Region == it.Region, "YOU DO IT WRONG!");
                return Offset - it.Offset;
            }

            TSelf& operator+=(ptrdiff_t off) {
                Offset += off;
                return *this;
            }

            TSelf& operator-=(ptrdiff_t off) {
                return this->operator+=(-off);
            }

            TSelf& operator++() {
                return this->operator+=(1);
            }
            TSelf& operator--() {
                return this->operator+=(-1);
            }

            TSelf operator+(ptrdiff_t off) {
                TSelf res = *this;
                res += off;
                return res;
            }

            TSelf operator-(ptrdiff_t off) {
                return this->operator+(-off);
            }

            TSelf operator++(int) {
                TConstIterator it = *this;
                this->operator+=(1);
                return it;
            }

            TSelf operator--(int) {
                TConstIterator it = *this;
                this->operator+=(-1);
                return it;
            }
        };
    }

    template <typename TIdx, typename TReg>
    struct TGetFromRegion {
        TIdx operator()(const TReg& reg, size_t off) const {
            return reg[off];
        }
    };

    template <typename TIndex, typename TIdx = ui64>
    class TIndexedBase : TNonCopyable {
    protected:
        TIndex IndexRegion;
        TStringBuf DataRegion;

        TVector<TIdx> IndexBuffer;
        TBuffer DataBuffer;
        TGetFromRegion<TIdx, TIndex> GetFromRegion;
        bool Building = false;

        size_t GetOffset(size_t idx) const {
            if (Building) {
                return IndexBuffer[idx];
            } else {
                return GetFromRegion(IndexRegion, idx);
            }
        }

        virtual void DoCommit() = 0;

    public:
        using TConstIterator = NPrivate::TConstIterator<TIndexedBase<TIndex, TIdx>>;

        static TStringBuf AsDataRegion(const TBuffer& b) {
            return TStringBuf{b.data(), b.size()};
        }

    public:
        virtual ~TIndexedBase() {
        }

        TStringBuf Get(size_t idx) const {
            TStringBuf r = Data();
            ui64 off0 = GetOffset(idx);
            ui64 off1 = idx + 1 < Size() ? GetOffset(idx + 1) : r.size();

            return r.SubStr(off0, off1 - off0);
        }

        TStringBuf operator[](size_t idx) const {
            return Get(idx);
        }

        size_t Size() const {
            return Building ? IndexBuffer.size() : IndexRegion.size();
        }

        TStringBuf Data() const {
            return Building ? AsDataRegion(DataBuffer) : DataRegion;
        }

        bool Empty() const {
            return Building ? IndexBuffer.empty() : IndexRegion.empty();
        }

        TStringBuf PushBack(TStringBuf r) {
            Building = true;
            const size_t off = DataBuffer.Size();
            IndexBuffer.push_back(off);
            DataBuffer.Append(r.data(), r.size());
            return AsDataRegion(DataBuffer).Skip(off);
        }

        void Commit() {
            DoCommit();
            Building = false;
        }

        void Clear() {
            ClearNoReset();
            TVector<TIdx>().swap(IndexBuffer);
            DataBuffer.Reset();
        }

        void ClearNoReset() {
            Building = false;
            NAccessors::Clear(IndexRegion);
            DataRegion.Clear();
            IndexBuffer.clear();
            DataBuffer.Clear();
        }

        TConstIterator Begin() const {
            return TConstIterator(this, 0);
        }

        TConstIterator End() const {
            return TConstIterator(this, Size());
        }

        virtual void Encode(TBuffer& r) const = 0;
        virtual void Decode(TStringBuf r) = 0;
    };

}

namespace std {
    template <typename TIndexedRegion>
    struct iterator_traits<
        typename NIndexedRegion::NPrivate::TConstIterator<TIndexedRegion>> {
        using difference_type = ptrdiff_t;
        using value_type = TStringBuf;
        using iterator_category = random_access_iterator_tag;
    };

}
