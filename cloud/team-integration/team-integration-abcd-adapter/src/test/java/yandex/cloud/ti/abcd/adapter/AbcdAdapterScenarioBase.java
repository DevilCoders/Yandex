package yandex.cloud.ti.abcd.adapter;

import lombok.Getter;
import lombok.extern.log4j.Log4j2;
import yandex.cloud.scenario.contract.AbstractContractScenario;
import yandex.cloud.ti.yt.abcd.client.MockTeamAbcdClient;

import ru.yandex.intranet.d.backend.service.provider_proto.AccountsServiceGrpc;

@Log4j2
public abstract class AbcdAdapterScenarioBase extends AbstractContractScenario {

    @Getter
    private final AccountsServiceGrpc.AccountsServiceBlockingStub accountsServiceClient = TestAbcdAdapterConfiguration.createTestAccountsServiceClient();

    protected AbcdAdapterScenarioSuite.Context getCtx() {
        return (AbcdAdapterScenarioSuite.Context) ctx;
    }

    public MockTeamAbcdClient getMockTeamAbcdClient() {
        return getCtx().getMockTeamAbcdClient();
    }

}
