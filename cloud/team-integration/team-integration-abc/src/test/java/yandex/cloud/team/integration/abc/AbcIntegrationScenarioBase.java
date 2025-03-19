package yandex.cloud.team.integration.abc;

import java.util.concurrent.TimeUnit;

import lombok.Getter;
import lombok.extern.log4j.Log4j2;
import org.jetbrains.annotations.NotNull;
import org.junit.After;
import org.junit.Before;
import org.junit.rules.DisableOnDebug;
import org.junit.rules.Timeout;
import yandex.cloud.converter.AnyConverter;
import yandex.cloud.iam.repository.tracing.QueryTraceContext;
import yandex.cloud.priv.operation.PO;
import yandex.cloud.priv.team.integration.v1.PTIAS;
import yandex.cloud.scenario.contract.AbstractContractScenario;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepository;
import yandex.cloud.ti.abc.repo.InMemoryAbcIntegrationRepository;
import yandex.cloud.ti.yt.abc.client.MockTeamAbcClient;
import yandex.cloud.ti.yt.abcd.client.MockTeamAbcdClient;

@Log4j2
public abstract class AbcIntegrationScenarioBase extends AbstractContractScenario {

    @Getter
    private final TestAbcIntegrationClient abcServiceClient =
            TestAbcIntegrationClient.inProcess(TestAbcIntegrationConfiguration.NAME);


    public AbcIntegrationScenarioBase() {
        // override AbstractContractScenario.globalTimeout
        globalTimeout = new DisableOnDebug(new Timeout(30L, TimeUnit.SECONDS));
    }


    @Before
    public void resetAbcIntegrationRepository() {
        InMemoryAbcIntegrationRepository abcIntegrationRepository = (InMemoryAbcIntegrationRepository) getAbcIntegrationRepository();
        abcIntegrationRepository.clear();
    }

    @After
    public void clearMockTeamAbcClient() {
        getMockTeamAbcClient().clearServices();
    }


    protected @NotNull String waitOperationAndGetCloudId(
            @NotNull PO.Operation operation
    ) {
        return waitOperationAndGetResponse(operation).getCloudId();
    }

    protected @NotNull CreateCloudResponse waitOperationAndGetResponse(
            @NotNull PO.Operation operation
    ) {
        return QueryTraceContext.withoutQueryTrace(() -> AnyConverter.<CreateCloudResponse>unpack(getAbcServiceClient().getAbcServiceOps()
                        .join(operation)
                        .getResponse())
                .bind(PTIAS.CreateCloudResponse.class,
                        it -> new CreateCloudResponse(it.getCloudId(), it.getDefaultFolderId())
                )
                .build());
    }

    protected AbcIntegrationScenarioSuite.Context getCtx() {
        return (AbcIntegrationScenarioSuite.Context) ctx;
    }

    protected MockTeamAbcClient getMockTeamAbcClient() {
        return getCtx().getMockTeamAbcClient();
    }

    protected MockTeamAbcdClient getMockTeamAbcdClient() {
        return getCtx().getMockTeamAbcdClient();
    }

    protected AbcIntegrationRepository getAbcIntegrationRepository() {
        return getCtx().getAbcIntegrationRepository();
    }

}
