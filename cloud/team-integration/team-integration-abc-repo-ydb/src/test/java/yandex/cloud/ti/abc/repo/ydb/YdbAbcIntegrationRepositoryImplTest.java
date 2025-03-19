package yandex.cloud.ti.abc.repo.ydb;

import java.util.List;
import java.util.function.Supplier;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import yandex.cloud.iam.repository.tracing.QueryTrace;
import yandex.cloud.iam.repository.tracing.QueryTraceRule;
import yandex.cloud.team.integration.repository.RepositoryRule;
import yandex.cloud.team.integration.repository.TeamIntegrationEntitiesHelper;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepository;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepositoryContractTest;

public class YdbAbcIntegrationRepositoryImplTest extends AbcIntegrationRepositoryContractTest {

    @Rule
    public final RepositoryRule repositoryRule = new RepositoryRule(TeamIntegrationEntitiesHelper.collectEntities(List.of(
            AbcIntegrationEntities.entities
    )));

    @Rule
    public final QueryTraceRule queryTraceRule = new QueryTraceRule(this);


    private AbcIntegrationRepository abcIntegrationRepository;


    @Before
    public void prepareAbcServiceCloudRepository() {
        abcIntegrationRepository = new YdbAbcIntegrationRepositoryImpl();
    }

    @Override
    public @NotNull AbcIntegrationRepository getAbcIntegrationRepository() {
        return abcIntegrationRepository;
    }

    @Override
    protected <T> @Nullable T tx(@NotNull Supplier<T> supplier) {
        return repositoryRule.tx(supplier);
    }

    @Override
    @Test
    @QueryTrace({"createAbcServiceCloud", "findAbcServiceCloudByCloudId"})
    public void createAndFindAbcServiceCloudByCloudId() {
        super.createAndFindAbcServiceCloudByCloudId();
    }

    @Override
    @Test
    @QueryTrace({"findAbcServiceCloudByCloudId"})
    public void findAbcServiceCloudByCloudId() {
        super.findAbcServiceCloudByCloudId();
    }

    @Override
    @Test
    @QueryTrace({"createAbcServiceCloud", "findAbcServiceCloudByAbcServiceId"})
    public void createAndFindAbcServiceCloudByAbcServiceId() {
        super.createAndFindAbcServiceCloudByAbcServiceId();
    }

    @Override
    @Test
    @QueryTrace({"findAbcServiceCloudByAbcServiceId"})
    public void findAbcServiceCloudByAbcServiceId() {
        super.findAbcServiceCloudByAbcServiceId();
    }

    @Override
    @Test
    @QueryTrace({"createAbcServiceCloud", "findAbcServiceCloudByAbcServiceSlug"})
    public void createAndFindAbcServiceCloudByAbcServiceSlug() {
        super.createAndFindAbcServiceCloudByAbcServiceSlug();
    }

    @Override
    @Test
    @QueryTrace({"findAbcServiceCloudByAbcServiceSlug"})
    public void findAbcServiceCloudByAbcServiceSlug() {
        super.findAbcServiceCloudByAbcServiceSlug();
    }

    @Override
    @Test
    @QueryTrace({"createAbcServiceCloud", "findAbcServiceCloudByAbcdFolderId"})
    public void createAndFindAbcServiceCloudByAbcdFolderId() {
        super.createAndFindAbcServiceCloudByAbcdFolderId();
    }

    @Override
    @Test
    @QueryTrace({"findAbcServiceCloudByAbcdFolderId"})
    public void findAbcServiceCloudByAbcdFolderId() {
        super.findAbcServiceCloudByAbcdFolderId();
    }

    @Override
    @Test
    @QueryTrace({"createAbcServiceCloud", "listAbcServiceClouds"})
    public void listAbcServiceClouds() {
        super.listAbcServiceClouds();
    }

    @Override
    @Test
    @QueryTrace({"createAbcServiceCloud", "listAbcServiceCloudsWithPaging"})
    public void listAbcServiceCloudsWithPaging() {
        super.listAbcServiceCloudsWithPaging();
    }


    @Override
    @Test
    @QueryTrace({"saveCreateOperation", "findCreateOperationByAbcServiceId"})
    public void saveAndFindCreateOperationByAbcServiceId() {
        super.saveAndFindCreateOperationByAbcServiceId();
    }

    @Override
    @Test
    @QueryTrace({"findCreateOperationByAbcServiceId"})
    public void findCreateOperationByAbcServiceId() {
        super.findCreateOperationByAbcServiceId();
    }

    @Override
    @Test
    @QueryTrace({"saveStubOperation", "findStubOperationByOperationId"})
    public void saveAndFindStubOperationByStubOperationId() {
        super.saveAndFindStubOperationByStubOperationId();
    }

    @Override
    @Test
    @QueryTrace({"findStubOperationByOperationId"})
    public void findStubOperationByOperationId() {
        super.findStubOperationByOperationId();
    }

}
