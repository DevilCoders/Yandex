#pragma once

#include <library/cpp/deprecated/accessors/accessors.h>

#include <library/cpp/packers/packers.h>

namespace NCodecTrie {
    template <typename Ta, typename TPacker, bool EnsureCopy>
    struct TMemoryRepresentation {
    private:
        template <typename Tb>
        struct TMemoryRepresentationBase {
            TStringBuf CheckOwnsMemory(TStringBuf r) {
                if (!TMemoryTraits<Tb>::OwnsMemory) {
                    Buffer->Assign(r.begin(), r.end());
                    return TStringBuf{Buffer->data(), Buffer->size()};
                } else {
                    return r;
                }
            }

            TBuffer* Buffer = nullptr;
        };

        template <typename Tb>
        struct TDefaultMemoryRepresentation : TMemoryRepresentationBase<Tb> {
            using TElementType = typename TMemoryTraits<Tb>::TElementType;

            TStringBuf ToMemoryRegion(const Tb& t) {
                TStringBuf mem{(const char*)NAccessors::Begin(t), (const char*)NAccessors::End(t)};
                return EnsureCopy ? this->CheckOwnsMemory(mem) : mem;
            }

            void FromMemoryRegion(Tb& t, TStringBuf r) {
                if (EnsureCopy) {
                    r = this->CheckOwnsMemory(r);
                }

                NAccessors::Assign(t, (const TElementType*)r.begin(), (const TElementType*)r.end());
            }
        };

        template <typename Tb, typename TPack>
        struct TCompactTriePackerMemoryRepresentation : TMemoryRepresentationBase<Tb> {
            TPack Packer;

            TStringBuf ToMemoryRegion(const Tb& t) {
                size_t sz = Packer.MeasureLeaf(t);
                this->Buffer->Resize(sz);
                Packer.PackLeaf(this->Buffer->Begin(), t, sz);
                return TStringBuf{(*this->Buffer).data(), (*this->Buffer).size()};
            };

            void FromMemoryRegion(Tb& t, TStringBuf r) {
                if (EnsureCopy) {
                    r = this->CheckOwnsMemory(r);
                }

                Packer.UnpackLeaf(r.begin(), t);
            }
        };

        using TRepr = typename std::conditional<
            TMemoryTraits<Ta>::ContinuousMemory,
            TDefaultMemoryRepresentation<Ta>,
            TCompactTriePackerMemoryRepresentation<Ta, TPacker>>::type;

        TRepr Repr;

    public:
        TMemoryRepresentation(TBuffer* b = nullptr) {
            Repr.Buffer = b;
        }

        TStringBuf ToMemoryRegion(const Ta& t) {
            return Repr.ToMemoryRegion(t);
        }

        void FromMemoryRegion(Ta& t, TStringBuf r) {
            Repr.FromMemoryRegion(t, r);
        }
    };
}
