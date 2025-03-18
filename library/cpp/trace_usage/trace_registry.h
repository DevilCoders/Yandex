#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>

namespace NTraceUsage {
    // Simplified, overoptimized and overencapsulated move-only TBuffer for ui8 data
    class TReportBlank {
    private:
        ui8* Start = nullptr;
        ui8* Current = nullptr;
        const ui8* End = nullptr;

        TReportBlank(size_t size);

    public:
        TReportBlank() = default;
        ~TReportBlank();

        // move only
        TReportBlank(const TReportBlank&) = delete;
        TReportBlank& operator=(const TReportBlank&) = delete;

        TReportBlank(TReportBlank&& other) noexcept
            : Start(other.Start)
            , Current(other.Current)
            , End(other.End)
        {
            other.Start = nullptr;
            other.Current = nullptr;
            other.End = nullptr;
        }

        ui8* GetCurrent(size_t requiredSize) noexcept;
        void SetCurrent(ui8* current) noexcept {
            Current = current;
        }
        void Reset() noexcept {
            Current = Start;
        }

        void Swap(TReportBlank& other) noexcept {
            DoSwap(other.Start, Start);
            DoSwap(other.Current, Current);
            DoSwap(other.End, End);
        }

        TArrayRef<const ui8> GetDataRegion() const noexcept {
            return TArrayRef<const ui8>(Start, Current);
        }

        static TReportBlank CreateBlank() {
            return TReportBlank(1 << 13);
        }
    };

    class ITraceRegistry: public TAtomicRefCount<ITraceRegistry> {
    private:
        static ui8* WriteCommonData(ui8* start, TMaybe<ui64> reportContext) noexcept;
        inline void WriteDummyEvent(const ui32 fieldNumber) noexcept;
        inline void ReportSmallFunctionEvent(TStringBuf functionName, const ui32 fieldNumber) noexcept;
        inline void ReportFunctionEvent(TStringBuf functionName, const ui32 fieldNumber) noexcept;

        friend class TReportContext;
        void ReportChildContextCreation(ui64 parentContext, ui64 childContext) noexcept;

    protected:
        virtual TReportBlank AcquireReportBlank() noexcept = 0;
        virtual void ReleaseReportBlank(TReportBlank reportBlank) noexcept = 0;

        virtual void ConsumeReport(TArrayRef<const ui8> report) noexcept = 0; // Blank lies on stack or other unowned memory

    public:
        virtual ~ITraceRegistry() = default;

        void ReportOpenFunction(TStringBuf functionName) noexcept;
        void ReportCloseFunction(TStringBuf functionName) noexcept;

        void ReportStartAcquiringMutex() noexcept;
        void ReportAcquiredMutex() noexcept;
        void ReportStartReleasingMutex() noexcept;
        void ReportReleasedMutex() noexcept;

        void ReportStartWaitEvent() noexcept;
        void ReportTimeoutWaitEvent() noexcept;
        void ReportFinishWaitEvent() noexcept;

        class TReportConstructor: private TMoveOnly {
        private:
            TIntrusivePtr<ITraceRegistry> Registry;
            TReportBlank Blank;

            void AddTagImpl(TStringBuf name) noexcept;
            void AddParamImpl(TStringBuf name, TStringBuf value) noexcept;
            void AddParamImpl(TStringBuf name, ui64 value) noexcept;
            void AddParamImpl(TStringBuf name, i64 value) noexcept;
            void AddParamImpl(TStringBuf name, float value) noexcept;
            void AddParamImpl(TStringBuf name, double value) noexcept;

            void AddParamImpl(TStringBuf name, ui32 value) noexcept {
                AddParamImpl(name, (ui64)value);
            }
            void AddParamImpl(TStringBuf name, i32 value) noexcept {
                AddParamImpl(name, (i64)value);
            }
            void AddParamImpl(TStringBuf name, ui16 value) noexcept {
                AddParamImpl(name, (ui64)value);
            }
            void AddParamImpl(TStringBuf name, i16 value) noexcept {
                AddParamImpl(name, (i64)value);
            }
            void AddParamImpl(TStringBuf name, bool value) noexcept {
                AddParamImpl(name, (ui64)value);
            }
            void AddParamImpl(TStringBuf name, const char* value) noexcept {
                AddParamImpl(name, TStringBuf(value));
            }

            void AddFactorsImpl(const TStringBuf* names, const float* values, size_t size) noexcept;
            void DropRegistry();

            explicit TReportConstructor(TMaybe<ui64> reportContext);

            friend class TReportContext;
            TReportConstructor(TIntrusivePtr<ITraceRegistry> registry, TMaybe<ui64> reportContext);

        public:
            TReportConstructor()
                : TReportConstructor{Nothing()}
            {
            }

            ~TReportConstructor();

            TReportConstructor(TReportConstructor&&) noexcept;

            // this code has side effects, it writes blank report to logs:
            // { TReportConstructor emptyReport; }
            //
            // It means that for any reasonable implementation of operator=
            // { TReportConstructor rep; rep = std::move(other); }
            // may not be equivalent to { TReportConstructor rep(std::move(other)); }
            // so I decided to prohibit this usage
            TReportConstructor& operator=(TReportConstructor&&) = delete;

            TReportConstructor& AddTag(TStringBuf name) noexcept {
                if (Registry) {
                    AddTagImpl(name);
                }
                return *this;
            }

            template <typename TValue>
            TReportConstructor& AddParam(TStringBuf name, TValue&& value) noexcept {
                if (Registry) {
                    AddParamImpl(name, std::forward<TValue>(value));
                }
                return *this;
            }

            template <size_t size>
            TReportConstructor& AddFactors(const TStringBuf (&names)[size], const float (&values)[size]) noexcept {
                if (size && Registry) {
                    AddFactorsImpl(names, values, size);
                }
                return *this;
            }
            TReportConstructor& AddFactors(TArrayRef<const TStringBuf> names, TArrayRef<const float> values) noexcept {
                Y_VERIFY(names.size() == values.size(), "Different size of factor names & values arrays");
                if (Registry && names.size()) {
                    AddFactorsImpl(names.data(), values.data(), names.size());
                }
                return *this;
            }

            void Swap(TReportConstructor& other) {
                DoSwap(other.Registry, Registry);
                DoSwap(other.Blank, Blank);
            }
        };
    };

    using TTraceRegistryPtr = TIntrusivePtr<ITraceRegistry>;

}
