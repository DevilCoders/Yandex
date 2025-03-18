#pragma once

#include <util/generic/bitops.h>
#include <util/generic/utility.h>
#include <util/system/align.h>

namespace NCalloc {

    constexpr char CALLOC_PROFILER_SIGNATURE[] = {'A', 'L', 'L', 'O', 'C', 'H', 'D', 'R'};
    constexpr size_t SIGNATURE_MASK = 0xFULL << 56;
    constexpr i64 DIFF_ENCODING_START = 0x0000000020000000;

    enum EAllocType : ui64 {
        NotInit,
        BAlloc = 0x8ULL << 56/* "b" */,
        LF = 0x9ULL << 56/* "lf" */,
#ifndef _musl_
        System = 0xaULL << 56/* "libc" */,
#endif
        Mmap = 0xbULL << 56/* "mmap" */
    };

    constexpr size_t STACK_TRACE_BUFFER_SIZE = Max<ui8>() + 1;
    extern thread_local ui8 threadLocalStackTraceSize;
    extern thread_local ui8 threadLocalStackTraceBuffer[STACK_TRACE_BUFFER_SIZE];

    namespace NPrivate {
        // https://github.com/lemire/streamvbyte/blob/master/src/streamvbyte_zigzag.c
        Y_FORCE_INLINE ui64 ZigZagEncode(i64 value) {
            return (value + value) ^ (value >> 63);
        }

        // https://github.com/git/git/blob/1c4b6604126781361fe89d88ace70f53079fbab5/varint.c
        Y_FORCE_INLINE size_t VarintEncode(ui64 value, ui8* buf, size_t bufSize) {
            ui8 varInt[10];
            size_t pos = sizeof(varInt) - 1;
            varInt[pos] = value & 0x7F;
            while (value >>= 7) {
                --pos;
                --value;
                varInt[pos] = 0x80 | (value & 0x7F);
            }
            const size_t varIntSize = sizeof(varInt) - pos;
            if (bufSize >= varIntSize) {
                memcpy(buf, varInt + pos, varIntSize);
                return varIntSize;
            }
            return 0;
        }

        struct TAllocHeader {
            void* Block;
            size_t AllocSize;

            Y_FORCE_INLINE void Encode(void* block, size_t size, size_t signature) {
                Block = block;
                AllocSize = size | signature;
            }
            Y_FORCE_INLINE void* GetDataPtr() {
                return this + 1;
            }

            static Y_FORCE_INLINE size_t GetHeaderSize() {
                return sizeof(TAllocHeader);
            }
            static Y_FORCE_INLINE void Clear(void* /*dataPtr*/) {
            }

            static Y_FORCE_INLINE EAllocType GetAllocType(void* dataPtr) {
                return (EAllocType)(FromDataPtr(dataPtr)->AllocSize & SIGNATURE_MASK);
            }
            static Y_FORCE_INLINE size_t GetAllocSize(void* dataPtr) {
                return FromDataPtr(dataPtr)->AllocSize & ~SIGNATURE_MASK;
            }
            static Y_FORCE_INLINE void* GetBlock(void* dataPtr) {
                return FromDataPtr(dataPtr)->Block;
            }

            static Y_FORCE_INLINE void CollectStackTrace() {
            }

            static Y_FORCE_INLINE void* AlignHeader(void* dataPtr, size_t alignment) {
                void* alignedPtr = AlignUp(dataPtr, alignment);
                if (alignedPtr != dataPtr) {
                    const ui8* oldAllocHeader = (const ui8*)dataPtr - sizeof(TAllocHeader);
                    ui8* newAllocHeader = (ui8*)alignedPtr - sizeof(TAllocHeader);
                    memmove(newAllocHeader, oldAllocHeader, sizeof(TAllocHeader));
                }
                return alignedPtr;
            }

        private:
            static Y_FORCE_INLINE TAllocHeader* FromDataPtr(void* ptr) {
                return (TAllocHeader*)ptr - 1;
            }
        };

        struct TAllocHeaderForProfiler {
            Y_FORCE_INLINE void Encode(void* block, size_t size, size_t signature) {
                TAllocHeader* allocHeader = (TAllocHeader*)((ui8*)this + GetHeaderSize()) - 1;
                allocHeader->Encode(block, size, signature);

                ui8* profilerData = (ui8*)this + GetHeaderSize() - UnalignedSize();
                memcpy(profilerData, threadLocalStackTraceBuffer, threadLocalStackTraceSize);
                profilerData[threadLocalStackTraceSize] = threadLocalStackTraceSize;
                memcpy(profilerData + threadLocalStackTraceSize + 1, CALLOC_PROFILER_SIGNATURE, sizeof(CALLOC_PROFILER_SIGNATURE));
            }
            Y_FORCE_INLINE void* GetDataPtr() {
                return (ui8*)this + GetHeaderSize();
            }

            static Y_FORCE_INLINE size_t GetHeaderSize() {
                return AlignUp(UnalignedSize(), sizeof(TAllocHeader));
            }
            static Y_FORCE_INLINE void Clear(void* dataPtr) {
                ui8* signature = (ui8*)dataPtr - sizeof(TAllocHeader) - sizeof(CALLOC_PROFILER_SIGNATURE);
                memset(signature, 0, sizeof(CALLOC_PROFILER_SIGNATURE));
            }

            static Y_FORCE_INLINE EAllocType GetAllocType(void* dataPtr) {
                return TAllocHeader::GetAllocType(dataPtr);
            }
            static Y_FORCE_INLINE size_t GetAllocSize(void* dataPtr) {
                return TAllocHeader::GetAllocSize(dataPtr);
            }
            static Y_FORCE_INLINE void* GetBlock(void* dataPtr) {
                return TAllocHeader::GetBlock(dataPtr);
            }

            static Y_FORCE_INLINE void CollectStackTrace() {
                struct TStackFrame {
                    const TStackFrame* rbp;
                    const ui64 rip;
                };
                const TStackFrame* stackFrame;
                asm ("mov %%rbp,%0" : "=rm"(stackFrame) ::);
                i64 prevRip = DIFF_ENCODING_START;
                size_t bufferPos = 0;
                while (stackFrame) {
                    // Отнимаем единицу от адреса возврата чтобы адрес в стеке указывал в область правильного символа
                    const i64 rip = (i64)(stackFrame->rip - 1);
                    const i64 ripDiff = rip - prevRip;
                    prevRip = rip;
                    // +1 чтобы не записывать 0, который является признаком конца данных
                    const ui64 zigZaggedRipDiff = ZigZagEncode(ripDiff) + 1;
                    const size_t varIntSize = VarintEncode(
                        zigZaggedRipDiff,
                        threadLocalStackTraceBuffer + bufferPos,
                        sizeof(threadLocalStackTraceBuffer) - bufferPos
                    );
                    if (Y_UNLIKELY(!varIntSize)) {
                        break;
                    }
                    bufferPos += varIntSize;
                    stackFrame = stackFrame->rbp;
                }
                threadLocalStackTraceSize = bufferPos;
            }

            static Y_FORCE_INLINE void* AlignHeader(void* dataPtr, size_t alignment) {
                void* alignedPtr = AlignUp(dataPtr, alignment);
                if (alignedPtr != dataPtr) {
                    const size_t allocHeaderSize = GetHeaderSize();
                    ui8* oldAllocHeader = (ui8*)dataPtr - allocHeaderSize;
                    ui8* newAllocHeader = (ui8*)alignedPtr - allocHeaderSize;
                    memmove(newAllocHeader, oldAllocHeader, allocHeaderSize);
                    // Очистка старой сигнатуры
                    memset(oldAllocHeader, 0, Min(allocHeaderSize, static_cast<size_t>(newAllocHeader - oldAllocHeader)));
                }
                return alignedPtr;
            }

        private:
            static Y_FORCE_INLINE size_t UnalignedSize() {
                return threadLocalStackTraceSize + 1 + sizeof(CALLOC_PROFILER_SIGNATURE) + sizeof(TAllocHeader);
            }
        };
    }  // namespace NPrivate

#ifdef CALLOC_PROFILER
    using TAllocHeader = NPrivate::TAllocHeaderForProfiler;
#else
    using TAllocHeader = NPrivate::TAllocHeader;
#endif
    constexpr size_t ALLOC_HEADER_ALIGNMENT = sizeof(NPrivate::TAllocHeader);

    static_assert(IsPowerOf2(ALLOC_HEADER_ALIGNMENT));

}  // namespace NCalloc
