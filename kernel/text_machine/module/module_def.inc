#include <kernel/text_machine/module/module.h>

// Uncomment to enable (a lot of) debug output to Cerr
//
//#define ENABLE_TEXT_MACHINE_DEBUG

#ifdef ENABLE_TEXT_MACHINE_DEBUG
#    define MACHINE_LOG(Scope, Event, Arg) \
         Cerr << (Scope) << " " << Event << " " << (Arg) << Endl;
#else
#    define MACHINE_LOG(...) // no op
#endif

#define MACHINE_PARTS(MachineName) \
    namespace N##MachineName##Parts

#define MACHINE_MOTOR \
    class TMachineBase {}; \
    template <typename M> \
    class TMotor : public TMachineBase, public M

#define UNIT_FAMILY(UnitFamily) \
    struct UnitFamily \
    { \
        struct TId {}; \
    };

#define UNIT(UnitName, ...) \
    struct UnitName \

#define UNIT_INFO_BEGIN(UnitName, ...) \
    class UnitName ## Info \
        : public ::NModule::TUnitInfoBase \
    { \
    public: \
        UnitName ## Info(__VA_ARGS__) {

#define UNIT_INFO_END() \
        } \
    };

#define UNIT_FAMILY_INFO_BEGIN(UnitFamily, ...) \
    class UnitFamily ## Info \
        : public ::NModule::TUnitInfoBase \
    { \
    public: \
        UnitFamily ## Info(__VA_ARGS__) {

#define UNIT_FAMILY_INFO_END() \
        } \
    };

#define UNIT_METHOD_INSTANTIATE_ARG(MethodName, ArgTypeDescr, ArgName, InstValues) \
    SetUnitMethodInfoArg( \
        this->Methods, \
        TStringBuf(#MethodName), \
        &ArgTypeDescr::Instance(), \
        TStringBuf(#ArgName), \
        ::NModule::TValuesList(InstValues).Values)

#define UNIT_METHOD_FORWARD_ARG(MethodName, ArgTypeDescr, ArgName) \
    ForwardUnitMethodInfoArg( \
        this->Methods, \
        TStringBuf(#MethodName), \
        &ArgTypeDescr::Instance(), \
        TStringBuf(#ArgName))

#if defined(TEXT_MACHINE_STATIC_CPP)
#    define UNIT_REGISTER(DomainName, UnitName, InfoName, ...) \
        static ::NModule::TUnitRegistrator Register ## UnitName { \
            MakeHolder<InfoName>(__VA_ARGS__), \
            TStringBuf(#DomainName), \
            ::NModule::GetNameByUnitCppName(TStringBuf(#UnitName)), \
            TStringBuf(#UnitName) \
        };

#    define UNIT_FAMILY_REGISTER(DomainName, UnitFamily, InfoName, ...) \
        static ::NModule::TUnitRegistrator Register ## UnitFamily { \
            MakeHolder<InfoName>(__VA_ARGS__), \
            TStringBuf(#DomainName), \
            ::NModule::GetNameByFamilyCppName(TStringBuf(#UnitFamily)), \
            TStringBuf(#UnitFamily) \
        };
#else
#    define UNIT_REGISTER(DomainName, UnitName, InfoName, ...)
#    define UNIT_FAMILY_REGISTER(DomainName, UnitFamily, InfoName, ...)
#endif

#define UNIT_STATE \
    struct TState; \
    using TId = ::NModule::TStateIdAlias<TState>; \
    struct TState \
        : public ::NModule::TStateBase<TState> \

#define UNIT_FAMILY_STATE(UnitFamily) \
    using TId = ::NModule::TFamilyStateIdAlias<UnitFamily>; \
    struct TState \
        : public ::NModule::TFamilyStateBase<UnitFamily> \

#define UNIT_PROCESSOR \
    using TLocalState = TState; \
    template <typename P> \
    class TProcessor \
        : public P::TStub \
        , public TState

#define UNIT_PROCESSOR_CUSTOM(ProcessorName) \
    using TLocalState = TState; \
    template <typename P> \
    class ProcessorName \
        : public P::TStub \
        , public TState

#define UNIT_PROCESSOR_STATIC \
    struct TProcessorStaticState

#define UNIT_PROCESSOR_STATIC_INIT \
    TProcessorStaticState()

#define UNIT_PROCESSOR_METHODS \
    public: \
        using TTraits = P; \
        using TMachine = typename P::TMachine; \
        using TAccess = typename P::TAccess; \
        using TProcessorState = TLocalState; \
        using TId = typename TProcessorState::TId; \
        template <typename U> \
        using TGetProc = typename TAccess::template TProcById<typename U::TId>; \
        template <typename U> \
        using TGetState = typename TGetProc<U>::TProcessorState; \
        template <typename U> \
        using TGetStatic = typename TGetProc<U>::TProcessorStaticState; \
        struct TProcessorStaticState; \
        template <int N, typename... Args> \
        struct TRequire {}; \
    public: \
        using P::TStub::ScatterStatic; \
        using P::TStub::Scatter; \
        Y_FORCE_INLINE TMachine& Machine() { \
            return static_cast<TMachine&>(*this); \
        } \
        Y_FORCE_INLINE const TMachine& Machine() const { \
            return static_cast<const TMachine&>(*this); \
        } \
        Y_FORCE_INLINE TProcessorState& Vars() { \
            return Machine(); \
        } \
        Y_FORCE_INLINE const TProcessorState& Vars() const { \
            return Machine(); \
        } \
        Y_FORCE_INLINE static const TProcessorStaticState& Static() { \
            static TProcessorStaticState state{}; \
            return state; \
        } \
        template <typename U> \
        Y_FORCE_INLINE const TGetProc<U>& Proc() const { \
            return Machine(); \
        } \
        template <typename U> \
        Y_FORCE_INLINE const TGetState<U>& Vars() const { \
            return Machine(); \
        } \
        template <typename U> \
        Y_FORCE_INLINE static const TGetStatic<U>& Static() { \
            return TGetProc<U>::Static(); \
        } \
        Y_FORCE_INLINE static size_t UnitId() { \
            return P::UnitId; \
        } \
        template <typename U> \
        Y_FORCE_INLINE static size_t UnitId() { \
            return TGetProc<U>::TTraits::UnitId; \
        } \
        Y_FORCE_INLINE void SetUnitEnabled(bool f) { \
            P::TStub::SetUnitEnabled(f); \
        } \
        Y_FORCE_INLINE bool IsUnitEnabled() const { \
            return P::TStub::IsUnitEnabled(); \
        } \
        Y_FORCE_INLINE void SaveToJson(NJson::TJsonValue& value) const { \
            TString stringId = TProcessorState::StringId; \
            Y_ASSERT(stringId); \
            TProcessorState::SaveToJson(value[stringId]); \
        } \

#define REQUIRE_UNIT(UnitName, ...) \
    template <typename... Args> \
    struct TRequire<0, Args...> \
        : public ::NModule::TRequireAllOf<::NModule::TRequireList<__VA_ARGS__>, Args...> \
    {};

#define REQUIRE_MACHINE(MachineName, Type) \
    static_assert(std::is_base_of<N##MachineName##Parts::TMachineBase, Type>::value, \
        "type " #Type " should be a machine " #MachineName);

#define SAVE_JSON_VAR(Json, Var) \
    Json[#Var] = ::NModule::JsonValue(Var)
