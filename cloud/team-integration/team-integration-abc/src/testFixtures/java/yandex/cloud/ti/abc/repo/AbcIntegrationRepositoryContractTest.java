package yandex.cloud.ti.abc.repo;

import java.util.function.Supplier;

import org.assertj.core.api.Assertions;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import org.junit.Test;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.AbcServiceCloudCreateOperationReference;
import yandex.cloud.ti.abc.AbcServiceCloudStubOperationReference;
import yandex.cloud.ti.abc.TestAbcServiceClouds;
import yandex.cloud.ti.yt.abc.client.TestTeamAbcServices;

public abstract class AbcIntegrationRepositoryContractTest {

    protected abstract @NotNull AbcIntegrationRepository getAbcIntegrationRepository();

    protected abstract <T> @Nullable T tx(@NotNull Supplier<T> supplier);


    @Test
    public void createAndFindAbcServiceCloudByCloudId() {
        AbcIntegrationRepository abcIntegrationRepository = getAbcIntegrationRepository();
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();

        Assertions.assertThat(tx(() -> abcIntegrationRepository.createAbcServiceCloud(
                        abcServiceCloud
                )))
                .isEqualTo(abcServiceCloud);

        Assertions.assertThat(tx(() -> abcIntegrationRepository.findAbcServiceCloudByCloudId(
                        abcServiceCloud.cloudId()
                )))
                .isEqualTo(abcServiceCloud);
    }

    @Test
    public void findAbcServiceCloudByCloudId() {
        AbcIntegrationRepository abcIntegrationRepository = getAbcIntegrationRepository();
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();

        Assertions.assertThat(tx(() -> abcIntegrationRepository.findAbcServiceCloudByCloudId(
                        abcServiceCloud.cloudId()
                )))
                .isNull();
    }

    @Test
    public void createAndFindAbcServiceCloudByAbcServiceId() {
        AbcIntegrationRepository abcIntegrationRepository = getAbcIntegrationRepository();
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();

        Assertions.assertThat(tx(() -> abcIntegrationRepository.createAbcServiceCloud(
                        abcServiceCloud
                )))
                .isEqualTo(abcServiceCloud);

        Assertions.assertThat(tx(() -> abcIntegrationRepository.findAbcServiceCloudByAbcServiceId(
                        abcServiceCloud.abcServiceId()
                )))
                .isEqualTo(abcServiceCloud);
    }

    @Test
    public void findAbcServiceCloudByAbcServiceId() {
        AbcIntegrationRepository abcIntegrationRepository = getAbcIntegrationRepository();
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();

        Assertions.assertThat(tx(() -> abcIntegrationRepository.findAbcServiceCloudByAbcServiceId(
                        abcServiceCloud.abcServiceId()
                )))
                .isNull();
    }

    @Test
    public void createAndFindAbcServiceCloudByAbcServiceSlug() {
        AbcIntegrationRepository abcIntegrationRepository = getAbcIntegrationRepository();
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();

        Assertions.assertThat(tx(() -> abcIntegrationRepository.createAbcServiceCloud(
                        abcServiceCloud
                )))
                .isEqualTo(abcServiceCloud);

        Assertions.assertThat(tx(() -> abcIntegrationRepository.findAbcServiceCloudByAbcServiceSlug(
                        abcServiceCloud.abcServiceSlug()
                )))
                .isEqualTo(abcServiceCloud);
    }

    @Test
    public void findAbcServiceCloudByAbcServiceSlug() {
        AbcIntegrationRepository abcIntegrationRepository = getAbcIntegrationRepository();
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();

        Assertions.assertThat(tx(() -> abcIntegrationRepository.findAbcServiceCloudByAbcServiceSlug(
                        abcServiceCloud.abcServiceSlug()
                )))
                .isNull();
    }

    @Test
    public void createAndFindAbcServiceCloudByAbcdFolderId() {
        AbcIntegrationRepository abcIntegrationRepository = getAbcIntegrationRepository();
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();

        Assertions.assertThat(tx(() -> abcIntegrationRepository.createAbcServiceCloud(
                        abcServiceCloud
                )))
                .isEqualTo(abcServiceCloud);

        Assertions.assertThat(tx(() -> abcIntegrationRepository.findAbcServiceCloudByAbcdFolderId(
                        abcServiceCloud.abcdFolderId()
                )))
                .isEqualTo(abcServiceCloud);
    }

    @Test
    public void findAbcServiceCloudByAbcdFolderId() {
        AbcIntegrationRepository abcIntegrationRepository = getAbcIntegrationRepository();
        AbcServiceCloud abcServiceCloud = TestAbcServiceClouds.nextAbcServiceCloud();

        Assertions.assertThat(tx(() -> abcIntegrationRepository.findAbcServiceCloudByAbcdFolderId(
                        abcServiceCloud.abcdFolderId()
                )))
                .isNull();
    }

    @Test
    public void listAbcServiceClouds() {
        AbcIntegrationRepository abcIntegrationRepository = getAbcIntegrationRepository();
        AbcServiceCloud abcServiceCloud1 = TestAbcServiceClouds.nextAbcServiceCloud();
        AbcServiceCloud abcServiceCloud2 = TestAbcServiceClouds.nextAbcServiceCloud();

        AbcServiceCloud entity1 = tx(() -> abcIntegrationRepository.createAbcServiceCloud(abcServiceCloud1));
        AbcServiceCloud entity2 = tx(() -> abcIntegrationRepository.createAbcServiceCloud(abcServiceCloud2));

        ListPage<AbcServiceCloud> page = tx(() -> abcIntegrationRepository.listAbcServiceClouds(100, null));
        Assertions.<AbcServiceCloud>assertThat(page.items())
                .containsExactly(entity1, entity2);
        Assertions.assertThat(page.nextPageToken()).isNull();
    }

    @Test
    public void listAbcServiceCloudsWithPaging() {
        AbcIntegrationRepository abcIntegrationRepository = getAbcIntegrationRepository();
        AbcServiceCloud abcServiceCloud1 = TestAbcServiceClouds.nextAbcServiceCloud();
        AbcServiceCloud abcServiceCloud2 = TestAbcServiceClouds.nextAbcServiceCloud();

        AbcServiceCloud entity1 = tx(() -> abcIntegrationRepository.createAbcServiceCloud(abcServiceCloud1));
        AbcServiceCloud entity2 = tx(() -> abcIntegrationRepository.createAbcServiceCloud(abcServiceCloud2));

        ListPage<AbcServiceCloud> page1 = tx(() -> abcIntegrationRepository.listAbcServiceClouds(1, null));
        Assertions.<AbcServiceCloud>assertThat(page1.items())
                .containsExactly(entity1);
        Assertions.assertThat(page1.nextPageToken()).isNotNull();

        ListPage<AbcServiceCloud> page2 = tx(() -> abcIntegrationRepository.listAbcServiceClouds(1, page1.nextPageToken()));
        Assertions.<AbcServiceCloud>assertThat(page2.items())
                .containsExactly(entity2);
        Assertions.assertThat(page2.nextPageToken()).isNull();
    }


    @Test
    public void saveAndFindCreateOperationByAbcServiceId() {
        AbcIntegrationRepository abcIntegrationRepository = getAbcIntegrationRepository();
        long abcServiceId = TestTeamAbcServices.nextAbcServiceId();
        Operation.Id createOperationId = new Operation.Id("createOperationId");
        AbcServiceCloudCreateOperationReference expected = new AbcServiceCloudCreateOperationReference(
                abcServiceId,
                createOperationId
        );

        Assertions.assertThat(tx(() -> abcIntegrationRepository.saveCreateOperation(
                        abcServiceId,
                        createOperationId
                )))
                .isEqualTo(expected);

        Assertions.assertThat(tx(() -> abcIntegrationRepository.findCreateOperationByAbcServiceId(
                        abcServiceId
                )))
                .isEqualTo(expected);
    }

    @Test
    public void findCreateOperationByAbcServiceId() {
        AbcIntegrationRepository abcIntegrationRepository = getAbcIntegrationRepository();
        long abcServiceId = TestTeamAbcServices.nextAbcServiceId();

        Assertions.assertThat(tx(() -> abcIntegrationRepository.findCreateOperationByAbcServiceId(
                        abcServiceId
                )))
                .isNull();
    }

    @Test
    public void saveAndFindStubOperationByStubOperationId() {
        AbcIntegrationRepository abcIntegrationRepository = getAbcIntegrationRepository();
        Operation.Id stubOperationId = new Operation.Id("stubOperationId");
        Operation.Id createOperationId = new Operation.Id("createOperationId");
        AbcServiceCloudStubOperationReference expected = new AbcServiceCloudStubOperationReference(
                stubOperationId,
                createOperationId
        );

        Assertions.assertThat(tx(() -> abcIntegrationRepository.saveStubOperation(
                        stubOperationId,
                        createOperationId
                )))
                .isEqualTo(expected);

        Assertions.assertThat(tx(() -> abcIntegrationRepository.findStubOperationByOperationId(
                        stubOperationId
                )))
                .isEqualTo(expected);
    }

    @Test
    public void findStubOperationByOperationId() {
        AbcIntegrationRepository abcIntegrationRepository = getAbcIntegrationRepository();
        Operation.Id stubOperationId = new Operation.Id("stubOperationId");

        Assertions.assertThat(tx(() -> abcIntegrationRepository.findStubOperationByOperationId(
                        stubOperationId
                )))
                .isNull();
    }

}
