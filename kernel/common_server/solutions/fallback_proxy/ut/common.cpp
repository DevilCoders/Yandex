#include "common.h"
#include <kernel/common_server/rt_background/processes/queue_executor/process.h>
#include <kernel/common_server/solutions/fallback_proxy/src/rt_background/queue_resender/process.h>

bool TFallbackTestBase::FillSpecialDBFeatures(NServerTest::TConfigGenerator& configGenerator) const {
    configGenerator.SetDBHost("sas-waniis89zubqxw1v.db.yandex.net");
    configGenerator.SetDBPort(6432);
    configGenerator.SetDBName("risk-fintech-tests");
    configGenerator.SetDBUser("tests-user");
    configGenerator.SetPPass("risk-fintech-tests");
    return true;
}

void TFallbackTestBase::OnTestStart() {
    TTagStorageCustomization::WriteOldBinaryData = false;
    TTagStorageCustomization::ReadOldBinaryData = false;
    TTagStorageCustomization::WriteNewBinaryData = false;
    TTagStorageCustomization::ReadNewBinaryData = true;
    TTagStorageCustomization::WritePackedBinaryData = true;
    TFallbackServerGuard::TConfigGenerator configGenerator;
    TBase::Initialize(configGenerator);
}

void TFallbackTestBase::PrepareSettings(TVector<NFrontend::TSetting>& /*settings*/) {
}

void TFallbackTestBase::PrepareBackgrounds(TVector<TRTBackgroundProcessContainer>& rtbg) {
    TBase::PrepareBackgrounds(rtbg);
    {
        auto processSettings = MakeHolder<NCS::NFallbackProxy::TResenderProcess>();
        processSettings->SetPeriod(TDuration::Seconds(3));
        processSettings->SetQueueId("pq");
        processSettings->SetSenderId("self");
        processSettings->SetDropOnCannotProcess(false);
        TRTBackgroundProcessContainer process(processSettings.Release());
        process.SetEnabled(true);
        process.SetName("resender");
        rtbg.emplace_back(std::move(process));
    }
}

TString TFallbackConfigGenerator::GetExternalQueuesConfiguration() const {
    TStringBuilder sb;
    sb << "<pq>" << Endl;
    sb << "    DBName: main-db" << Endl;
    sb << "    Type: db" << Endl;
    sb << "</pq>" << Endl;
    return sb;
}

TString TFallbackConfigGenerator::GetProcessorsConfiguration() const {
    static const TString permissionsConfig = R"(
                <test/handler/xxx>
                    ProcessorType: miracle
                </test/handler/xxx>
                <test_status/handler/xxx>
                    ProcessorType: miracle
                </test_status/handler/xxx>
                <proxy/$forward_url>
                    ProcessorType: fallback-proxy-writer-processor
                    TargetApiName: self
                    InputPathForMessageId: external_content_id.value
                    OutputPathForMessageId: expected_content_id.value
                    PersistentQueueId: pq
                </proxy/$forward_url>
                <proxy/status/by_message_id>
                    ProcessorType: fallback-proxy-status-processor
                    PersistentQueueId: pq
                    TargetApiName: self
                    InputPathForMessageId: status.expected_content_id
                    ReplyCode: 203
                    ReplyTemplate: {"status" : {"bbb" : "$MESSAGE_ID"}}
                    DefaultUrlParams: forward_url=test/handler/xxx;
                </proxy/status/by_message_id>
    )";
    return permissionsConfig;
}

TString TFallbackConfigGenerator::GetAuthConfiguration() const {
    TStringStream ss;
    ss << "<fake>" << Endl;
    ss << "Type: fake" << Endl;
    ss << "</fake>" << Endl;

    return ss.Str();
}

TString TFallbackConfigGenerator::GetCustomManagersConfiguration() const {
    static const TString permissionsConfig = R"(
                <RolesManager>
                   Type: configured
                   <Items>
                       <admin-settings>
                           type: settings
                           actions: add,modify,observe
                       </admin-settings>
                       <admin-rt_background>
                           type: rt_background
                           actions: add,modify,observe
                       </admin-rt_background>
                   </Items>
                   <Roles>
                       <admin>
                           Items: admin-settings, admin-rt_background
                       </admin>
                   </Roles>
                </RolesManager>

                <ExternalQueues>
                    <fake_persistent_queue>
                        Type: fake
                    </fake_persistent_queue>
                </ExternalQueues>

                <Emulations>
                    Type: configured
                    <Cases>
                    </Cases>
                </Emulations>

                <ResourcesManagerConfig>
                    DBName: main-db
                </ResourcesManagerConfig>

                <PropositionsManagerConfig>
                    DBName: main-db
                </PropositionsManagerConfig>

                <ObfuscatorManagerConfig>
                    DBName: main-db
                </ObfuscatorManagerConfig>

                <SnapshotsController>
                    DBName: main-db
                </SnapshotsController>

                <PermissionsManager>
                   Type: configured
                   DefaultUser: default
                   <Users>
                       <default>
                           Roles: admin
                       </default>
                       <admin>
                           Roles: admin
                       </admin>
                       <test_bank>
                           Roles: admin
                       </test_bank>
                   </Users>
                </PermissionsManager>
    )";
    TStringStream ss;
    ss << "<UsersManager>" << Endl;
    ss << "    DBName: main-db" << Endl;
    ss << "    Type: transparent" << Endl;
    ss << "</UsersManager>" << Endl;

    ss << permissionsConfig << Endl;
    return ss.Str();
}

TString TFallbackConfigGenerator::GetExternalApiConfiguration() const {
    static const TString apiConfig = R"(
            <self>
                 ApiHost: localhost
                 ApiPort: ${BasePort}
                 Https: 0
                 <RequestConfig>
                     GlobalTimeout: 300000
                     TimeoutSendingms: 1000
                     TimeoutConnectms : 1000
                     MaxAttempts: 1
                     TasksCheckIntervalms: 500
                 </RequestConfig>
            </self>
    )";
    return apiConfig;
}
