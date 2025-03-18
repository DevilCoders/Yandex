#pragma once

#include <util/random/random.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>
#include <util/digest/city.h>
#include <util/generic/vector.h>
#include <util/generic/algorithm.h>
#include <util/ysaveload.h>
#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/memory/blob.h>
#include <util/stream/mem.h>
#include <util/stream/format.h>

namespace NMinHash {
    class TVectorRank: public TVector<ui32> {
    public:
        inline TVectorRank()
            : TVector<ui32>()
        {
        }
        TVectorRank(const ui32* first, const ui32* last)
            : TVector<ui32>(first, last)
        {
        }
        ui32 Get(ui32 pos) const {
            return LowerBound(begin(), end(), pos) - begin();
        }
        ui64 Space() const {
            return size() * sizeof(ui32) * 8;
        }
        ui64 Size() const {
            return size();
        }
        void Swap(TVectorRank& other) {
            TVector<ui32>::swap(other);
        }
    };

}

template <>
class TSerializer<NMinHash::TVectorRank>: public TVectorSerializer<NMinHash::TVectorRank> {
};

namespace NMinHash {
    struct TItem {
        ui32 First;
        ui32 Second;
        ui32 Fpr;
    };

    class TByteCountingOutput: public IOutputStream {
    public:
        inline TByteCountingOutput(IOutputStream* slave) noexcept
            : Slave_(slave)
            , Length_(0)
        {
        }
        ~TByteCountingOutput() override {
        }
        inline ui64 Written() const noexcept {
            return Length_;
        }

    private:
        void DoWrite(const void* buf, size_t len) override {
            Slave_->Write(buf, len);
            Length_ += len;
        }

    private:
        IOutputStream* Slave_;
        ui64 Length_;
    };

    class TByteCountingInput: public IInputStream {
    public:
        inline TByteCountingInput(IInputStream* slave) noexcept
            : Slave_(slave)
            , Length_(0)
        {
        }
        ~TByteCountingInput() override {
        }
        inline ui64 Read() const noexcept {
            return Length_;
        }

    private:
        size_t DoRead(void* buf, size_t len) override {
            len = Slave_->Read(buf, len);
            Length_ += len;
            return len;
        }
        size_t DoSkip(size_t len) override {
            len = Slave_->Skip(len);
            Length_ += len;
            return len;
        }

    private:
        IInputStream* Slave_;
        ui64 Length_;
    };

    enum EHashType {
        HT_UNKNOWN = 0,
        HT_SINGLE = 1,
        HT_DISTR = 2
    };

    struct alignas(16) TChdMinHashFuncHdr {
        static const ui64 SIGNATURE;
        static const ui8 VERSION;

        ui64 Signature;
        i32 Type;
        ui8 Version;

        TChdMinHashFuncHdr(int type)
        {
            memset(this, 0, sizeof(TChdMinHashFuncHdr));
            Signature = SIGNATURE;
            Type = type;
            Version = VERSION;
        }

        ui8 Verify(int type) {
            if (Signature != SIGNATURE)
                ythrow yexception() << "Input has wrong signature: " << Hex(Signature) << ", expected " << Hex(SIGNATURE);
            if (Type != type)
                ythrow yexception() << "Input has wrong hash type: " << Type << ", expected " << type;
            if (Version != VERSION)
                ythrow yexception() << "Input has wrong version: " << Version << ", expected " << VERSION;
            return Version;
        }
    };

    struct alignas(16) TTableHdr {
        static const ui64 SIGNATURE;
        static const ui8 VERSION;

        ui64 Signature;
        ui8 Version;

        TTableHdr()
        {
            memset(this, 0, sizeof(TTableHdr));
            Signature = SIGNATURE;
            Version = VERSION;
        }

        ui8 Verify() {
            if (Signature != SIGNATURE)
                ythrow yexception() << "Input has wrong signature: " << Hex(Signature) << ", expected " << Hex(SIGNATURE);
            if (Version != VERSION)
                ythrow yexception() << "Input has wrong version: " << Version << ", expected " << VERSION;
            return Version;
        }
    };

}

Y_DECLARE_PODTYPE(NMinHash::TChdMinHashFuncHdr);
Y_DECLARE_PODTYPE(NMinHash::TTableHdr);
