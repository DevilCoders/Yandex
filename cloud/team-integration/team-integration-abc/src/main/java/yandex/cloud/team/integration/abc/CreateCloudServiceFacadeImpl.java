package yandex.cloud.team.integration.abc;

import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.extern.log4j.Log4j2;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.service.OperationCallHandler;
import yandex.cloud.iam.service.TransactionHandler;
import yandex.cloud.model.operation.Operation;
import yandex.cloud.task.TaskProcessor;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepository;
import yandex.cloud.ti.yt.abc.client.TeamAbcClient;
import yandex.cloud.ti.yt.abc.client.TeamAbcService;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdClient;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdFolder;

@Log4j2
public class CreateCloudServiceFacadeImpl implements CreateCloudServiceFacade {

    private final @NotNull StubOperationService stubOperationService;
    private final @NotNull TeamAbcClient teamAbcClient;
    private final @NotNull TeamAbcdClient teamAbcdClient;
    private final @NotNull TaskProcessor taskProcessor;
    private final @NotNull AbcIntegrationRepository abcIntegrationRepository;


    public CreateCloudServiceFacadeImpl(
            @NotNull AbcIntegrationRepository abcIntegrationRepository,
            @NotNull StubOperationService stubOperationService,
            @NotNull TeamAbcClient teamAbcClient,
            @NotNull TeamAbcdClient teamAbcdClient,
            @NotNull TaskProcessor taskProcessor
    ) {
        this.abcIntegrationRepository = abcIntegrationRepository;
        this.stubOperationService = stubOperationService;
        this.teamAbcClient = teamAbcClient;
        this.teamAbcdClient = teamAbcdClient;
        this.taskProcessor = taskProcessor;
    }


    @Override
    public @NotNull Operation createByAbcServiceId(
            long abcServiceId
    ) {
        return create(AbcSource.fromId(abcServiceId));
    }

    @Override
    public @NotNull Operation createByAbcServiceSlug(
            @NotNull String abcServiceSlug
    ) {
        return create(AbcSource.fromSlug(abcServiceSlug));
    }

    @Override
    public @NotNull Operation createByAbcdFolderId(
            @NotNull String abcdFolderId
    ) {
        return create(AbcSource.fromFolderId(abcdFolderId));
    }

    private @NotNull Operation create(
            @NotNull AbcSource abcSource
    ) {
        var maybeOperation = TransactionHandler.runInTx(
                OperationCallHandler.getIdempotentOperationOrRun(() ->
                        new CloudToCreate(abcSource)
                                .throwOnAlreadyExists())
        );

        if (maybeOperation.isOperation()) {
            var operation = maybeOperation.getOperation();
            checkAbcServiceMatch(abcSource, (CreateCloudMetadata) operation.getMetadata());
            return operation;
        }

        var abcService = maybeOperation.getValue().getAbcService();
        var abcFolder = maybeOperation.getValue().getAbcdFolder();

        return TransactionHandler.runInTx(
                OperationCallHandler.withIdempotence(() -> {
                    new CloudToCreate(abcSource)
                            .throwOnAlreadyExists();

                    var mainOperation = stubOperationService.findMainOperation(abcService.id());
                    if (mainOperation.isEmpty()) {
                        // For the first request an asynchronous job will be started.
                        var createCloudOperation = new CreateCloudOperationMethod(taskProcessor, abcService, abcFolder)
                                .run();
                        stubOperationService.saveMainOperation(abcService.id(), createCloudOperation);
                        return createCloudOperation;
                    } else {
                        // A stub operation will be created for all others.
                        return stubOperationService.createStub(mainOperation.get());
                    }
                })
        );
    }

    /**
     * To be sure that caller is not using one idempotency key to create different clouds.
     *
     * @param abcSource id or slug of the new request
     * @param metadata existing operation metadata
     * @throws InvalidCreateRequestException if parameters do not match
     * @implNote it is ok to send a create request for abc_id=A and abc_slug=B in another request if both
     * are for same ABC service with id=A and slug=B. For such case, the parameters are considered equal.
     */
    private void checkAbcServiceMatch(
            @NotNull AbcSource abcSource,
            @NotNull CreateCloudMetadata metadata
    ) {
        var same = abcSource.isId() && abcSource.getId() == metadata.getAbcId()
                || abcSource.isSlug() && abcSource.getSlug().equals(metadata.getAbcSlug())
                || abcSource.isFolderId() && abcSource.getFolderId().equals(metadata.getAbcFolderId());
        if (!same) {
            throw InvalidCreateRequestException.parameterMismatch();
        }
    }

    class CloudToCreate {

        @NotNull AbcSource abcSource;

        public CloudToCreate(@NotNull AbcSource abcSource) {
            this.abcSource = abcSource;
        }

        /**
         * Fail fast if a cloud already created for the abc service.
         *
         * @return {@code this}
         */
        @NotNull CloudToCreate throwOnAlreadyExists() {
            AbcServiceCloud abcServiceCloud;
            if (abcSource.isId()) {
                abcServiceCloud = abcIntegrationRepository.findAbcServiceCloudByAbcServiceId(abcSource.getId());
            } else if (abcSource.isSlug()) {
                abcServiceCloud = abcIntegrationRepository.findAbcServiceCloudByAbcServiceSlug(abcSource.getSlug());
            } else {
                abcServiceCloud = abcIntegrationRepository.findAbcServiceCloudByAbcdFolderId(abcSource.getFolderId());
            }
            if (abcServiceCloud != null) {
                throw CloudAlreadyExistsException.forService(abcServiceCloud);
            }
            return this;
        }

        /**
         * Returns ABC service details.
         *
         * @return ABC service id, slug and name
         */
        @NotNull TeamAbcService getAbcService() {
            if (abcSource.isFolderId()) {
                var abcId = teamAbcdClient.getAbcdFolder(abcSource.getFolderId()).abcServiceId();
                return teamAbcClient.getAbcServiceById(abcId);
            }

            return abcSource.isId()
                    ? teamAbcClient.getAbcServiceById(abcSource.getId())
                    : teamAbcClient.getAbcServiceBySlug(abcSource.getSlug());
        }

        /**
         * Returns ABC folder details.
         *
         * @return ABC folder details
         */
        TeamAbcdFolder getAbcdFolder() {
            if (abcSource.isFolderId()) {
                TeamAbcdFolder abcdFolder = teamAbcdClient.getAbcdFolder(abcSource.getFolderId());
                if (!abcdFolder.defaultForService()) {
                    throw InvalidCreateRequestException.nonDefaultFolder(abcdFolder.id(), abcdFolder.abcServiceId());
                }
                return abcdFolder;
            } else {
                var abcServiceId = abcSource.isId()
                        ? abcSource.getId()
                        : teamAbcClient.getAbcServiceBySlug(abcSource.getSlug()).id();
                return teamAbcdClient.getDefaultAbcdFolder(abcServiceId);
            }
        }

    }

    @AllArgsConstructor(access = AccessLevel.PRIVATE)
    private static class AbcSource {

        long id;
        String folderId;
        String slug;

        boolean isId() {
            return id != 0;
        }

        boolean isSlug() {
            return slug != null && !slug.isEmpty();
        }

        boolean isFolderId() {
            return folderId != null && !folderId.isEmpty();
        }

        public long getId() {
            if (!isId()) {
                throw new IllegalStateException("Abc id is not set");
            }
            return id;
        }

        public String getSlug() {
            if (!isSlug()) {
                throw new IllegalStateException("Abc slug is not set");
            }
            return slug;
        }

        public String getFolderId() {
            if (!isFolderId()) {
                throw new IllegalStateException("Abc folder id is not set");
            }
            return folderId;
        }

        public static @NotNull AbcSource fromId(long id) {
            return new AbcSource(id, null, null);
        }

        public static @NotNull AbcSource fromSlug(
                @NotNull String slug
        ) {
            return new AbcSource(0, null, slug);
        }

        public static @NotNull AbcSource fromFolderId(
                @NotNull String folderId
        ) {
            return new AbcSource(0, folderId, null);
        }

    }

}
