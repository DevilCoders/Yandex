#include "evlog_descriptors.h"


namespace NAntiRobot {


namespace {


THashMap<TString, const NProtoBuf::Descriptor*> MakeEvlogTypeNameToDescriptor() {
    THashMap<TString, const NProtoBuf::Descriptor*> evlogTypeNameToDescriptor;

    for (const auto descriptor : {
        NAntirobotEvClass::TAntirobotFactors::descriptor(),
        NAntirobotEvClass::TBadRequest::descriptor(),
        NAntirobotEvClass::TBlockEvent::descriptor(),
        NAntirobotEvClass::TCacherFactors::descriptor(),
        NAntirobotEvClass::TCaptchaCheck::descriptor(),
        NAntirobotEvClass::TCaptchaImageError::descriptor(),
        NAntirobotEvClass::TCaptchaImageShow::descriptor(),
        NAntirobotEvClass::TCaptchaRedirect::descriptor(),
        NAntirobotEvClass::TCaptchaShow::descriptor(),
        NAntirobotEvClass::TCaptchaTokenExpired::descriptor(),
        NAntirobotEvClass::TCaptchaVoice::descriptor(),
        NAntirobotEvClass::TCaptchaVoiceIntro::descriptor(),
        NAntirobotEvClass::TCbbRuleParseResult::descriptor(),
        NAntirobotEvClass::TCbbRulesUpdated::descriptor(),
        NAntirobotEvClass::TGeneralMessage::descriptor(),
        NAntirobotEvClass::TRequestData::descriptor(),
        NAntirobotEvClass::TRequestGeneralMessage::descriptor(),
        NAntirobotEvClass::TVerochkaRecord::descriptor(),
        NAntirobotEvClass::TWizardError::descriptor(),
        NAntirobotEvClass::TCaptchaIframeShow::descriptor(),
    }) {
        evlogTypeNameToDescriptor[descriptor->name()] = descriptor;
    }

    return evlogTypeNameToDescriptor;
}


} // anonymous namespace


const THashMap<TString, const NProtoBuf::Descriptor*>& GetEvlogTypeNameToDescriptor() {
    static const THashMap<TString, const NProtoBuf::Descriptor*> evlogTypeNameToDescriptor =
        MakeEvlogTypeNameToDescriptor();

    return evlogTypeNameToDescriptor;
}


} // namespace NAntiRobot
