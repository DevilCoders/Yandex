#pragma once

//
// Autogenerated file
// This file was created from directives.proto and directives.jinja2 template
//
// Don't edit it manually!
// Please refer to doc: https://docs.yandex-team.ru/alice-scenarios/hollywood/main/codegen
// for more information about custom codegeneration
//

#include <util/generic/string.h>
#include <util/generic/vector.h>

//
// Forward declarations
//
namespace NAlice::NScenarios {
    class TScenarioResponseBody;
}
namespace NAlice::NScenarios {
    class TUpdateDatasyncDirective;
} // namespace NAlice::NScenarios
namespace NAlice::NScenarios {
    class TPushMessageDirective;
} // namespace NAlice::NScenarios
namespace NAlice::NScenarios {
    class TPersonalCardsDirective;
} // namespace NAlice::NScenarios
namespace NAlice::NScenarios {
    class TMementoChangeUserObjectsDirective;
} // namespace NAlice::NScenarios
namespace NAlice::NScenarios {
    class TSendPushDirective;
} // namespace NAlice::NScenarios
namespace NAlice::NScenarios {
    class TDeletePushesDirective;
} // namespace NAlice::NScenarios
namespace NAlice::NScenarios {
    class TPushTypedSemanticFrameDirective;
} // namespace NAlice::NScenarios
namespace NAlice::NScenarios {
    class TAddScheduleActionDirective;
} // namespace NAlice::NScenarios
namespace NAlice::NScenarios {
    class TSaveUserAudioDirective;
} // namespace NAlice::NScenarios
namespace NAlice::NScenarios {
    class TPatchAsrOptionsForNextRequestDirective;
} // namespace NAlice::NScenarios
namespace NAlice::NScenarios {
    class TCancelScheduledActionDirective;
} // namespace NAlice::NScenarios

namespace NAlice::NHollywoodFw {

//
// Forward declarations
//
class TRender;

//
// Internal interface for Directives containes
//
class TServerDirectiveWrapper {
public:
    explicit TServerDirectiveWrapper(const TString& name)
        : Name_(name)
    {
    }
    virtual ~TServerDirectiveWrapper() = default;
    virtual void Attach(NAlice::NScenarios::TScenarioResponseBody& response) = 0;
    const TString& GetName() const {
        return Name_;
    }
protected:
    TString Name_;
};
//
// Wrapper for directives
//
class TServerDirectivesWrapper {
public:
    //
    // Full list of directives
    //
    void AddUpdateDatasyncDirective(NAlice::NScenarios::TUpdateDatasyncDirective&& directive);
    void AddPushMessageDirective(NAlice::NScenarios::TPushMessageDirective&& directive);
    void AddPersonalCardsDirective(NAlice::NScenarios::TPersonalCardsDirective&& directive);
    void AddMementoChangeUserObjectsDirective(NAlice::NScenarios::TMementoChangeUserObjectsDirective&& directive);
    void AddSendPushDirective(NAlice::NScenarios::TSendPushDirective&& directive);
    void AddDeletePushesDirective(NAlice::NScenarios::TDeletePushesDirective&& directive);
    void AddPushTypedSemanticFrameDirective(NAlice::NScenarios::TPushTypedSemanticFrameDirective&& directive);
    void AddAddScheduleActionDirective(NAlice::NScenarios::TAddScheduleActionDirective&& directive);
    void AddSaveUserAudioDirective(NAlice::NScenarios::TSaveUserAudioDirective&& directive);
    void AddPatchAsrOptionsForNextRequestDirective(NAlice::NScenarios::TPatchAsrOptionsForNextRequestDirective&& directive);
    void AddCancelScheduledActionDirective(NAlice::NScenarios::TCancelScheduledActionDirective&& directive);

    // For internal using only
    void BuildAnswer(NAlice::NScenarios::TScenarioResponseBody& response);
private:
    TVector<std::shared_ptr<TServerDirectiveWrapper>> Directives_;
};

} // namespace NAlice::NHollywoodFw
