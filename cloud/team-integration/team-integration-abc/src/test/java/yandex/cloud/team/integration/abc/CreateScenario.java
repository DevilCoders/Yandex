package yandex.cloud.team.integration.abc;

import java.util.stream.Collectors;
import java.util.stream.IntStream;

import io.grpc.StatusRuntimeException;
import org.assertj.core.api.Assertions;
import org.junit.Test;
import yandex.cloud.converter.AnyConverter;
import yandex.cloud.priv.operation.PO;
import yandex.cloud.priv.team.integration.v1.PTIAS;
import yandex.cloud.scenario.DependsOn;
import yandex.cloud.ti.yt.abc.client.TeamAbcService;
import yandex.cloud.ti.yt.abc.client.TestTeamAbcServices;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdFolder;
import yandex.cloud.ti.yt.abcd.client.TestTeamAbcdFolders;

@DependsOn(AbcIntegrationScenarioSuite.class)
public class CreateScenario extends AbcIntegrationScenarioBase {

    @Test
    @Override
    public void main() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        getMockTeamAbcClient().addService(abcService);
        TeamAbcdFolder abcdFolder = TestTeamAbcdFolders.nextAbcdFolder(abcService.id());
        getMockTeamAbcdClient().addFolder(abcdFolder);

        var operation = getAbcServiceClient()
                .createByAbcFolderId(abcdFolder.id(), null);
        Assertions.assertThat(getAbcSlugFromMetadata(operation))
                .isEqualTo(abcService.slug());

        var cloudId = waitOperationAndGetCloudId(operation);
        Assertions.assertThat(cloudId)
                .isNotNull();
    }

    @Test
    public void testNotExistentById() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAbcServiceClient()
                        .createById(abcService.id(), null)
                )
                .withMessage("NOT_FOUND: ABC service for id %d not found".formatted(
                        abcService.id()
                ));
    }

    @Test
    public void testNotExistentByFolderId() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        String abcdFolderId = "abcd-folder-id-" + abcService.id(); // todo use TestAbcdFolders
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAbcServiceClient()
                        .createByAbcFolderId(abcdFolderId, null)
                )
                .withMessage("NOT_FOUND: ABC folder with id '%s' does not exist".formatted(
                        abcdFolderId
                ));
    }

    @Test
    public void testNotExistentBySlug() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAbcServiceClient()
                        .createBySlug(abcService.slug(), null)
                )
                .withMessage("NOT_FOUND: ABC service for slug '%s' not found".formatted(
                        abcService.slug()
                ));
    }

    @Test
    public void testCreateWithIdempotencyKey() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        getMockTeamAbcClient().addService(abcService);
        TeamAbcdFolder abcdFolder = TestTeamAbcdFolders.nextAbcdFolder(abcService.id());
        getMockTeamAbcdClient().addFolder(abcdFolder);
        String idempotencyKey = "idempotencyKey-" + abcService.id(); // todo extract method to generate IK

        var operation = getAbcServiceClient()
                .createByAbcFolderId(abcdFolder.id(), idempotencyKey);
        var operation2 = getAbcServiceClient()
                .createByAbcFolderId(abcdFolder.id(), idempotencyKey);
        Assertions.assertThat(operation.getId()).isEqualTo(operation2.getId());

        Assertions.assertThat(getAbcSlugFromMetadata(operation))
                .isNotNull()
                .isEqualTo(getAbcSlugFromMetadata(operation2))
                .isEqualTo(abcService.slug());

        var cloudId = waitOperationAndGetCloudId(operation);
        Assertions.assertThat(cloudId)
                .isNotNull();

        var cloudId2 = waitOperationAndGetCloudId(operation2);
        Assertions.assertThat(cloudId2)
                .isNotNull()
                .isEqualTo(cloudId);
    }

    @Test
    public void testCreateForDeletedService() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        getMockTeamAbcClient().addService(abcService);

        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAbcServiceClient()
                        .createById(abcService.id(), null)
                )
                .withMessage("NOT_FOUND: ABC service with id %d has no default folder",
                        abcService.id()
                );
    }

    @Test
    public void testCreateNonDefaultAbcFolder() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        getMockTeamAbcClient().addService(abcService);
        TeamAbcdFolder abcdFolder = TestTeamAbcdFolders.nextAbcdFolder(abcService.id(), false);
        getMockTeamAbcdClient().addFolder(abcdFolder);

        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAbcServiceClient()
                        .createByAbcFolderId(abcdFolder.id(), null)
                )
                .withMessage("INVALID_ARGUMENT: The folder '%s' is not default for the service %d".formatted(
                        abcdFolder.id(),
                        abcService.id()
                ));
    }

    @Test
    public void testCreateWithIdempotencyKeyForSameService() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        getMockTeamAbcClient().addService(abcService);
        TeamAbcdFolder abcdFolder = TestTeamAbcdFolders.nextAbcdFolder(abcService.id());
        getMockTeamAbcdClient().addFolder(abcdFolder);
        String idempotencyKey = "idempotencyKey-" + abcService.id(); // todo extract method to generate IK

        var operation = getAbcServiceClient()
                .createById(abcService.id(), idempotencyKey);
        var operation2 = getAbcServiceClient()
                .createBySlug(abcService.slug(), idempotencyKey);
        Assertions.assertThat(operation.getId()).isEqualTo(operation2.getId());

        Assertions.assertThat(getAbcSlugFromMetadata(operation))
                .isNotNull()
                .isEqualTo(getAbcSlugFromMetadata(operation2))
                .isEqualTo(abcService.slug());

        Assertions.assertThat(waitOperationAndGetCloudId(operation))
                .isNotNull()
                .isEqualTo(waitOperationAndGetCloudId(operation2));
    }

    @Test
    public void testCreateWithSameIdempotencyKeyButDifferentParameters() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        getMockTeamAbcClient().addService(abcService);
        TeamAbcdFolder abcdFolder = TestTeamAbcdFolders.nextAbcdFolder(abcService.id());
        getMockTeamAbcdClient().addFolder(abcdFolder);
        String idempotencyKey = "idempotencyKey-" + abcService.id(); // todo extract method to generate IK

        var operation = getAbcServiceClient()
                .createByAbcFolderId(abcdFolder.id(), idempotencyKey);
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAbcServiceClient()
                        .createByAbcFolderId(abcdFolder.id() + "-another", idempotencyKey)
                )
                .withMessage("INVALID_ARGUMENT: The set of parameters for requests with the same idempotence key must match");
        waitOperationAndGetCloudId(operation);
    }

    @Test
    public void testCreateRace() {
        TeamAbcService abcService = TestTeamAbcServices.nextAbcService();
        getMockTeamAbcClient().addService(abcService);
        TeamAbcdFolder abcdFolder = TestTeamAbcdFolders.nextAbcdFolder(abcService.id());
        getMockTeamAbcdClient().addFolder(abcdFolder);

        var numOperations = 10;
        // All concurrent cloud creation operations will be successful
        var ops = IntStream.range(0, numOperations)
                .mapToObj(__ -> getAbcServiceClient()
                        .createByAbcFolderId(abcdFolder.id(), null)
                )
                // TODO remove usage of peek()
                //  because stream can ignore that step if it decides that the final operation has predictable result
                .peek(op -> Assertions.assertThat(getAbcSlugFromMetadata(op))
                        .isEqualTo(abcService.slug())
                )
                .collect(Collectors.toSet());
        Assertions.assertThat(ops).hasSize(numOperations);

        // Ensure that only one cloud is created
        var uniqueIds = ops.stream()
                .map(getAbcServiceClient().getAbcServiceOps()::join)
                .peek(op -> Assertions.assertThat(op.getDone()).isTrue())
                .peek(op -> Assertions.assertThat(op.hasError()).isFalse())
                .map(CreateScenario::getCloudIdFromResponse)
                .collect(Collectors.toSet());
        Assertions.assertThat(uniqueIds).hasSize(1);
        // Subsequent cloud creation operations will fail
        var cloudId = uniqueIds.stream().findFirst().orElseThrow();
        Assertions.assertThatExceptionOfType(StatusRuntimeException.class)
                .isThrownBy(() -> getAbcServiceClient()
                        .createByAbcFolderId(abcdFolder.id(), null)
                )
                .withMessage("ALREADY_EXISTS: Cloud '%s' already created for ABC service id %d, slug '%s', abc folder '%s'".formatted(
                        cloudId,
                        abcService.id(),
                        abcService.slug(),
                        abcdFolder.id()
                ));
    }

    private static String getAbcFolderIdFromMetadata(PO.Operation operation) {
        return AnyConverter
                .<String>unpack(operation.getMetadata())
                .bind(PTIAS.CreateCloudMetadata.class, PTIAS.CreateCloudMetadata::getAbcFolderId)
                .build();
    }

    private static long getAbcIdFromMetadata(PO.Operation operation) {
        return AnyConverter
                .<Long>unpack(operation.getMetadata())
                .bind(PTIAS.CreateCloudMetadata.class, PTIAS.CreateCloudMetadata::getAbcId)
                .build();
    }

    private static String getAbcSlugFromMetadata(PO.Operation operation) {
        return AnyConverter
                .<String>unpack(operation.getMetadata())
                .bind(PTIAS.CreateCloudMetadata.class, PTIAS.CreateCloudMetadata::getAbcSlug)
                .build();
    }

    private static String getCloudIdFromResponse(PO.Operation operation) {
        return AnyConverter
                .<String>unpack(operation.getResponse())
                .bind(PTIAS.CreateCloudResponse.class, PTIAS.CreateCloudResponse::getCloudId)
                .build();
    }

}
