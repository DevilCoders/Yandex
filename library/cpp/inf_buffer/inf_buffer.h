#pragma once

/// @see https://kostik.at.yandex-team.ru/2087

#include <library/cpp/deprecated/atomic/atomic.h>

#include <util/generic/buffer.h>
#include <util/generic/mem_copy.h>
#include <util/generic/ptr.h>
#include <util/generic/array_ref.h>
#include <util/generic/strbuf.h>
#include <util/stream/output.h>

namespace NInfBuffer {
    template <typename TValue, typename TDeleter = TDelete>
    class TAtomicHolderPtr {
    private:
        TValue* volatile Value;

    public:
        TAtomicHolderPtr()
            : Value(nullptr)
        {
        }
        ~TAtomicHolderPtr() {
            THolder<TValue, TDeleter> extacted(AtomicGet(Value));
        }

        TValue* operator->() const noexcept {
            return AtomicGet(Value);
        }
        explicit operator bool() const noexcept {
            return AtomicGet(Value) != nullptr;
        }

        bool TrySet(THolder<TValue, TDeleter>& value) noexcept {
            if (AtomicCas(&Value, value.Get(), nullptr)) {
                Y_UNUSED(value.Release());
                return true;
            }
            return false;
        }

        THolder<TValue, TDeleter> Extract() noexcept {
            return THolder<TValue, TDeleter>(AtomicSwap(&Value, nullptr));
        }
    };

    class TMultiPartData {
    private:
        IOutputStream::TPart Part;
        const IOutputStream::TPart* Start = nullptr;
        const IOutputStream::TPart* const End = nullptr;

    public:
        TMultiPartData(const IOutputStream::TPart* start, size_t count)
            : Start(start)
            , End(start + count)
        {
            Next();
        }

        bool HasData() const noexcept {
            return Part.len;
        }
        void Next() noexcept {
            while (Start < End) {
                Part = *Start;
                ++Start;
                if (Part.len) {
                    return;
                }
            }
            Part = IOutputStream::TPart();
        }
        IOutputStream::TPart& MutablePart() noexcept {
            return Part;
        }
    };

    class TSinglePartData {
    private:
        IOutputStream::TPart Part;

    public:
        TSinglePartData(const void* buf, size_t len)
            : Part(buf, len)
        {
        }

        bool HasData() const noexcept {
            return Part.len;
        }
        void Next() noexcept {
            Part = IOutputStream::TPart();
        }
        IOutputStream::TPart& MutablePart() noexcept {
            return Part;
        }
    };

    class TNoData {
    public:
        static inline bool HasData() noexcept {
            return false;
        }
        [[noreturn]] void Next() {
            Y_FAIL();
        }
        [[noreturn]] IOutputStream::TPart& MutablePart() {
            Y_FAIL();
        }
    };

    template <typename TPartData>
    struct TWriteContext {
        IOutputStream& Output;
        size_t Shift = 0;
        TPartData PartData;

        TWriteContext(
            IOutputStream& output,
            size_t shift = 0,
            TPartData partData = TPartData())
            : Output(output)
            , Shift(shift)
            , PartData(partData)
        {
        }

        size_t FillRegion(TArrayRef<char> outRegion) noexcept {
            const size_t oldShift = Shift;
            while (true) {
                if (Shift >= outRegion.size()) {
                    Shift = 0;
                    return outRegion.size() - oldShift;
                }
                if (!PartData.HasData()) {
                    break;
                }
                IOutputStream::TPart& region = PartData.MutablePart();
                size_t toCopy = Min(region.len, outRegion.size() - Shift);
                ::MemCopy(outRegion.data() + Shift, (const char*)region.buf, toCopy);
                Shift += toCopy;
                if (toCopy < region.len) {
                    region = IOutputStream::TPart((const char*)region.buf + toCopy, region.len - toCopy);
                    Shift = 0;
                    return outRegion.size() - oldShift;
                }
                PartData.Next();
            }
            return Shift - oldShift;
        }

        // Is there anything to do
        explicit operator bool() const noexcept {
            return PartData.HasData();
        }
    };

    template <size_t bits>
    class TDataBlock {
    private:
        TAtomicCounter Written; // In fact written + carry bit
        char Reserved[248];     // Hack on CPU cache line size
        TMallocHolder<char> Data;

        template <typename TPartData>
        bool AddWritten(size_t written, char* const data, TWriteContext<TPartData>& context) {
            const size_t size = (size_t)1 << bits;
            if (Written.Add(written) == size + 1) { // size + carry bit
                // Bingo! Block full and ready for flushing
                context.Output.Write(data, size);
                Written.Sub(size + 1); // Restore initial state
                return true;
            }
            return false;
        }

    public:
        TDataBlock()
            : Data((char*)std::malloc((size_t)1 << bits))
        {
        }

        enum {
            DataBits = bits
        };

        // Returns true if data block flushed & could be collected for future use
        template <bool finishBit, typename TPartData>
        bool Write(bool carryBit, TWriteContext<TPartData>& context) {
            const size_t size = (size_t)1 << bits;
            size_t written1 = carryBit;
            char* const data = Data.Get();
            written1 += context.FillRegion(TArrayRef(data, size));
            if (written1) {
                if (AddWritten(written1, data, context)) {
                    return true;
                }
            }
            if (finishBit) {
                size_t written2 = Written.Val();
                Y_VERIFY(written2); // At least carry bit should be set
                context.Output.Write(data, written2 - 1);
                Written.Sub(written2);
                return true;
            }
            return false;
        }
    };

    template <typename TSubBlock, size_t bits>
    class TMetaDataBlock {
    private:
        TAtomicHolderPtr<TSubBlock> SubBlocks[(size_t)1 << bits];

    public:
        // Returns true if data block flushed & could be collected for future use
        template <bool finishBit, typename TPartData>
        bool Write(bool carryBit, TWriteContext<TPartData>& context) {
            size_t index = context.Shift / ((size_t)1 << TSubBlock::DataBits);
            context.Shift = context.Shift % ((size_t)1 << TSubBlock::DataBits);
            while (finishBit || carryBit || context) {
                if (index == ((size_t)1 << bits)) {
                    return carryBit;
                }
                size_t maxIndex = Min(((size_t)1 << bits), index + 10);
                auto collector = [&](THolder<TSubBlock> newBlock, size_t aheadIndex) {
                    while (aheadIndex < maxIndex && !SubBlocks[aheadIndex].TrySet(newBlock)) {
                        ++aheadIndex;
                    }
                };
                if (!SubBlocks[index]) {
                    collector(MakeHolder<TSubBlock>(), index);
                }
                if (carryBit = SubBlocks[index]->template Write<finishBit>(carryBit, context)) {
                    auto subBlock = SubBlocks[index].Extract();
                    if (finishBit) {
                        return true;
                    } else {
                        collector(std::move(subBlock), index + 1);
                    }
                }
                ++index;
            }
            return false;
        }

        enum {
            DataBits = TSubBlock::DataBits + bits
        };
    };

    using TDataBlock16 = TDataBlock<16>;
    using TMetaDataBlock32 = TMetaDataBlock<TDataBlock16, 16>;
    using TMetaDataBlock48 = TMetaDataBlock<TMetaDataBlock32, 16>;
    using TMetaDataBlock64 = TMetaDataBlock<TMetaDataBlock48, 16>;
    using TInfBufferImpl = std::conditional<sizeof(size_t) == 4, TMetaDataBlock32, TMetaDataBlock64>::type;

    class TInfBuffer: public IOutputStream {
    private:
        THolder<IOutputStream> Output;
        TInfBufferImpl InfBuffer;
        TAtomicCounter Shift;

        [[nodiscard]] size_t Reserve(size_t len) {
            return Shift.Add(len) - len;
        }
        template <bool finishBit, typename TPartData>
        void WriteContext(bool carryBit, TWriteContext<TPartData>& context) {
            InfBuffer.Write<finishBit>(carryBit, context);
        }

    public:
        TInfBuffer(THolder<IOutputStream> output)
            : Output(std::move(output))
        {
            TWriteContext<TNoData> context(
                /*Output=*/*Output,
                /*Shift=*/0);
            WriteContext</*finish=*/false>(/*carryBit=*/true, context);
        }
        ~TInfBuffer() try {
            Finish();
        } catch (...) {
        }
        void Finish() {
            if (Output) {
                TWriteContext<TNoData> context(
                    /*Output=*/*Output,
                    /*Shift=*/(size_t)Shift.Val());
                WriteContext</*finish=*/true>(/*carryBit=*/false, context);
                Output->Finish();
                Output.Destroy();
            }
        }

    protected:
        void DoWrite(const void* buf, size_t len) override {
            // Reserve space
            size_t shift = Reserve(len);

            // Configure data
            TWriteContext<TSinglePartData> context(
                /*Output=*/*Output,
                /*Shift=*/shift,
                /*partData=*/TSinglePartData(buf, len));
            WriteContext</*finish=*/false>(/*carryBit=*/false, context);
        }
        void DoWriteV(const TPart* parts, size_t count) override {
            // Reserve space
            size_t len = 0;
            for (size_t i = 0; i < count; ++i) {
                len += parts[i].len;
            }
            size_t shift = Reserve(len);

            // Configure data
            TWriteContext<TMultiPartData> context(
                /*Output=*/*Output,
                /*Shift=*/shift,
                /*partData=*/TMultiPartData(parts, count));
            WriteContext</*finish=*/false>(/*carryBit=*/false, context);
        }
    };

}
