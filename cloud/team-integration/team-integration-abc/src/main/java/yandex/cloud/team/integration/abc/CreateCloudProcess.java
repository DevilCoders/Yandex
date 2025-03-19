package yandex.cloud.team.integration.abc;

import java.time.Duration;
import java.util.Locale;
import java.util.Map;

import javax.inject.Inject;

import lombok.Getter;
import lombok.RequiredArgsConstructor;
import lombok.extern.log4j.Log4j2;
import org.jetbrains.annotations.NotNull;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperation;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationProcess;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationProcessState;
import yandex.cloud.iam.grpc.remoteoperation.RemoteOperationRetryConfig;
import yandex.cloud.iam.operation.OperationProcess;
import yandex.cloud.model.operation.OperationError;
import yandex.cloud.model.operation.OperationResponse;
import yandex.cloud.team.integration.abc.CreateCloudProcessState.Status;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepository;
import yandex.cloud.ti.billing.client.BillingAccount;
import yandex.cloud.ti.billing.client.BillingPrivateConfig;
import yandex.cloud.ti.billing.client.CreateBillingAccountRemoteOpCall;
import yandex.cloud.ti.rm.client.Cloud;
import yandex.cloud.ti.rm.client.CreateCloudRemoteOpCall;
import yandex.cloud.ti.rm.client.CreateFolderRemoteOpCall;
import yandex.cloud.ti.rm.client.Folder;
import yandex.cloud.ti.rm.client.ResourceManagerClient;
import yandex.cloud.ti.rm.client.SetAllCloudPermissionStagesRemoteOpCall;
import yandex.cloud.ti.yt.abc.client.TeamAbcService;
import yandex.cloud.ti.yt.abcd.client.TeamAbcdFolder;
import yandex.cloud.util.Json;

@Log4j2
@RequiredArgsConstructor
public class CreateCloudProcess implements OperationProcess<CreateCloudProcessState> {

    private static final int MAX_FOLDER_DESCRIPTION_LENGTH = 256;

    @Inject
    private static BillingPrivateConfig billingPrivateConfig;

    @Inject
    private static AbcIntegrationRepository abcIntegrationRepository;

    @Inject
    private static NamePolicyService namePolicyService;

    @Inject
    private static RemoteOperationRetryConfig defaultRetryConfig;
    @Inject
    private static ResourceManagerClient resourceManagerClient;
    @Inject
    private static AbcIntegrationProperties abcIntegrationProperties;

    private final @NotNull CreateCloudProcessState state;

    @Getter
    private final @NotNull TeamAbcService abcService;

    @Getter
    private final @NotNull TeamAbcdFolder abcdFolder;

    private RemoteOperationProcess<Cloud> remoteOperationProcessCreateCloud;
    private RemoteOperationProcess<RemoteOperation.Empty> remoteOperationProcessSetAllCloudPermissionStages;
    private RemoteOperationProcess<BillingAccount> remoteOperationProcessCreateBillingAccount;
    private RemoteOperationProcess<Folder> remoteOperationProcessCreateFolder;

    private boolean runPhaseExecuted;
    private CreateCloudProcessState nextState;

    @Override
    public CreateCloudProcessState getState() {
        return nextState != null ? nextState : state;
    }

    @Override
    public void prepare() {
        if (runPhaseExecuted) {
            throw new IllegalStateException(String.format("AbcService '%s' cloud creation: prepare phase cannot be after run phase!", getAbcService()));
        }

        log.trace("AbcService '{}' cloud creation, prepare phase, process status: {}", getAbcService(), getState().getStatus());

        switch (getState().getStatus()) {
            case CREATE_CLOUD -> prepareCreateCloud();
            case ENABLE_ALPHA_PERMISSIONS -> prepareSetAllCloudPermissionStages();
            case CREATE_BILLING_ACCOUNT -> prepareCreateBillingAccount();
            case CREATE_FOLDER -> prepareCreateFolder();
        }
    }

    @Override
    public void run() {
        if (runPhaseExecuted) {
            throw new IllegalStateException(String.format("AbsService '%s' cloud creation: run phase can only be executed once!", getAbcService()));
        }

        log.trace("AbcService '{}' cloud creation, run phase, process status: {}", getAbcService(), getState().getStatus());

        runPhaseExecuted = true;
        switch (getState().getStatus()) {
            case CREATE_CLOUD -> createCloud();
            case ENABLE_ALPHA_PERMISSIONS -> setAllCloudPermissionStages();
            case CREATE_BILLING_ACCOUNT -> createBillingAccount();
            case CREATE_FOLDER -> createFolder();
        }
    }

    @Override
    public CreateCloudProcessState done() {
        if (!runPhaseExecuted) {
            throw new IllegalStateException(String.format("AbcService '%s' cloud creation: done phase can only be after run phase!", getAbcService()));
        }

        log.trace("AbsService '{}' cloud creation, done phase, process status: {}", getAbcService(), getState().getStatus());

        switch (getState().getStatus()) {
            case CREATE_CLOUD -> nextState = doneCreateCloud();
            case ENABLE_ALPHA_PERMISSIONS -> nextState = doneSetAllCloudPermissionStages();
            case CREATE_BILLING_ACCOUNT -> nextState = doneCreateBillingAccount();
            case CREATE_FOLDER -> nextState = doneCreateFolder();
            case SAVE_RESULT -> nextState = doneSaveResult();
        }

        return getState();
    }

    @Override
    public boolean isDone() {
        return getState().isDone();
    }

    @Override
    public boolean isSucceeded() {
        return getState().isSucceeded();
    }

    @Override
    public boolean isFailed() {
        return getState().isFailed();
    }

    @Override
    public OperationError getOperationError() {
        if (!isFailed()) {
            throw new IllegalStateException(String.format("Must not get an error for a non failed cloud '%s' creation.", getAbcService()));
        }

        // XXX: map response status or remote operation error to operation error
        return new OperationError(OperationError.Code.INTERNAL, "");
    }

    @Override
    public OperationResponse getOperationResponse() {
        if (!isSucceeded()) {
            throw new IllegalStateException(String.format("Must not get a response for a failed cloud '%s' creation.", getAbcService()));
        }
        return new CreateCloudResponse(
                getState().getCloudId(),
                getState().getDefaultFolderId()
        );
    }

    @Override
    public Duration getRetryBackoff() {
        if (isDone()) {
            throw new IllegalStateException(String.format("The finished cloud '%s' creation cannot be retried.", getAbcService()));
        }

        RemoteOperationProcess<?> process = switch (getState().getStatus()) {
            case CREATE_CLOUD -> remoteOperationProcessCreateCloud;
            case CREATE_BILLING_ACCOUNT -> remoteOperationProcessCreateBillingAccount;
            case CREATE_FOLDER -> remoteOperationProcessCreateFolder;
            default -> null;
        };
        if (process != null && !process.isDone()) {
            return process.getRetryBackoff();
        }

        return Duration.ZERO;
    }

    private void prepareCreateCloud() {
        String normalizedName;
        if (getAbcdFolder().defaultForService()) {
            normalizedName = normalizeName("cloud", getAbcService().slug(), getAbcService().id());
        } else {
            var rawName = getAbcService().slug() + "-" + getAbcdFolder().name();
            normalizedName = normalizeName("cloud", rawName, getAbcService().id());
        }
        var labels = Map.of(
                "abcServiceId", Long.toString(getAbcService().id()),
                "abcServiceSlug", getAbcService().slug().toLowerCase(Locale.ENGLISH)
        );
        var description = Json.toJson(labels);

        remoteOperationProcessCreateCloud = new CreateCloudRemoteOperationProcess(
                getState().getCreateCloudState(),
                CreateCloudRemoteOpCall.createCreateCloud(
                        resourceManagerClient,
                        abcIntegrationProperties.abcServiceCloudOrganizationId(),
                        normalizedName,
                        description
                ),
                defaultRetryConfig
        );
    }

    @NotNull
    private static String normalizeName(
            @NotNull String type,
            @NotNull String name,
            @NotNull Object id
    ) {
        var normalizedName = namePolicyService.normalizeName(name);
        if (normalizedName.isEmpty()) {
            normalizedName = type + "-" + id;
            log.warn("ABC name {} contains no printable symbols. The {} name will be {}",
                    name, type, normalizedName);
        }
        return normalizedName;
    }

    private void createCloud() {
        remoteOperationProcessCreateCloud.run();
    }

    private CreateCloudProcessState doneCreateCloud() {
        var newState = getState().withCreateCloudState(remoteOperationProcessCreateCloud.getState());
        if (remoteOperationProcessCreateCloud.isDone()) {
            if (remoteOperationProcessCreateCloud.isSucceeded()) {
                var cloudId = remoteOperationProcessCreateCloud.getResult().id();
                log.debug("Created cloud {} for abc service {}", cloudId, getAbcService());

                if (billingPrivateConfig == null) {
                    log.debug("No billing endpoint is specified, creation of billing account was skipped");
                    return newState
                            .withCloudId(cloudId)
                            .withSetAllCloudPermissionStagesState(new RemoteOperationProcessState())
                            .withStatus(Status.ENABLE_ALPHA_PERMISSIONS);
                }

                return newState
                        .withCloudId(cloudId)
                        .withCreateBillingAccountState(new RemoteOperationProcessState())
                        .withStatus(Status.CREATE_BILLING_ACCOUNT);
            }

            // XXX: need save error info
            return newState.withStatus(Status.FAILED);
        }

        return newState;
    }

    private void prepareSetAllCloudPermissionStages() {
        remoteOperationProcessSetAllCloudPermissionStages = new SetAllCloudPermissionStagesRemoteOperationProcess(
                getState().getSetAllCloudPermissionStagesState(),
                SetAllCloudPermissionStagesRemoteOpCall.createSetAllCloudPermissionStages(
                        resourceManagerClient,
                        getState().getCloudId()
                ),
                defaultRetryConfig
        );
    }

    private void setAllCloudPermissionStages() {
        remoteOperationProcessSetAllCloudPermissionStages.run();
    }

    private CreateCloudProcessState doneSetAllCloudPermissionStages() {
        var newState = getState().withSetAllCloudPermissionStagesState(
                remoteOperationProcessSetAllCloudPermissionStages.getState()
        );
        if (remoteOperationProcessSetAllCloudPermissionStages.isDone()) {
            if (remoteOperationProcessSetAllCloudPermissionStages.isSucceeded()) {
                log.debug("Set all cloud permission stages for abc service {}", getAbcService());
                return newState
                        .withCreateFolderState(new RemoteOperationProcessState())
                        .withStatus(Status.CREATE_FOLDER);
            }

            // XXX: need save error info
            return newState.withStatus(Status.FAILED);
        }

        return newState;
    }

    private void prepareCreateBillingAccount() {
        // The billing account name should match ABC slug: https://st.yandex-team.ru/CLOUD-65731
        var billingAccountName = getAbcService().slug();
        var remoteOpCall =
                CreateBillingAccountRemoteOpCall.createCreateBillingAccount(billingAccountName,
                        getState().getCloudId());
        var state = getState().getCreateBillingAccountState();
        remoteOperationProcessCreateBillingAccount =
                new CreateBillingAccountRemoteOperationProcess(state, remoteOpCall, defaultRetryConfig);
    }

    private void createBillingAccount() {
        remoteOperationProcessCreateBillingAccount.run();
    }

    private CreateCloudProcessState doneCreateBillingAccount() {
        var newState = getState().withCreateBillingAccountState(remoteOperationProcessCreateBillingAccount.getState());
        if (remoteOperationProcessCreateBillingAccount.isDone()) {
            if (remoteOperationProcessCreateBillingAccount.isSucceeded()) {
                var billingAccountId = remoteOperationProcessCreateBillingAccount.getResult().getId();
                log.debug("Created billing account {} for abc service {}", billingAccountId, getAbcService());
                return newState
                        .withSetAllCloudPermissionStagesState(new RemoteOperationProcessState())
                        .withStatus(Status.ENABLE_ALPHA_PERMISSIONS);
            }

            if (remoteOperationProcessCreateBillingAccount.getOperationError().getMessage().startsWith("ALREADY_EXISTS")) {
                log.debug("A billing account already exists for abc service {}", getAbcService());
                // It is safe to continue in this case
                return newState
                        .withSetAllCloudPermissionStagesState(new RemoteOperationProcessState())
                        .withStatus(Status.ENABLE_ALPHA_PERMISSIONS);
            }

            // XXX: need save error info
            return newState.withStatus(Status.FAILED);
        }

        return newState;
    }

    private void prepareCreateFolder() {
        var normalizedName = normalizeName("folder", getAbcService().slug(), getAbcService().id());
        var labels = Map.of(
                "abc_service_id", Long.toString(getAbcService().id()),
                "abc_service_slug", namePolicyService.normalizeLabel(getAbcService().slug())
        );
        var description = createFolderDescription(getAbcService());

        remoteOperationProcessCreateFolder = new CreateFolderRemoteOperationProcess(
                getState().getCreateFolderState(),
                CreateFolderRemoteOpCall.createCreateFolder(
                        resourceManagerClient,
                        getState().getCloudId(),
                        normalizedName,
                        description,
                        labels
                ),
                defaultRetryConfig
        );
    }

    private String createFolderDescription(TeamAbcService abcService) {
        var description = "%s https://abc.yandex-team.ru/services/%s/".formatted(abcService.name(), abcService.slug());
        if (description.length() > MAX_FOLDER_DESCRIPTION_LENGTH) {
            description = "https://abc.yandex-team.ru/services/%s/".formatted(abcService.slug());
        }
        if (description.length() > MAX_FOLDER_DESCRIPTION_LENGTH) {
            description = "https://abc.yandex-team.ru/services/%s/".formatted(abcService.id());
        }
        return description;
    }

    private void createFolder() {
        remoteOperationProcessCreateFolder.run();
    }

    private CreateCloudProcessState doneCreateFolder() {
        var newState = getState().withCreateFolderState(remoteOperationProcessCreateFolder.getState());
        if (remoteOperationProcessCreateFolder.isDone()) {
            if (remoteOperationProcessCreateFolder.isSucceeded()) {
                var folderId = remoteOperationProcessCreateFolder.getResult().id();
                log.debug("Created folder {} for abc service {}", folderId, getAbcService());
                return newState
                        .withDefaultFolderId(folderId)
                        .withStatus(Status.SAVE_RESULT);
            }

            // XXX: need save error info
            return newState.withStatus(Status.FAILED);
        }

        return newState;
    }

    private CreateCloudProcessState doneSaveResult() {
        AbcServiceCloud abcServiceCloud = abcIntegrationRepository.findAbcServiceCloudByAbcServiceId(getAbcService().id());
        if (abcServiceCloud != null) {
            // This code is not intended to be reachable.
            // Clouds for ABC services shouldn't be created manually.
            throw CloudAlreadyExistsException.forService(abcServiceCloud);
        }

        abcIntegrationRepository.createAbcServiceCloud(new AbcServiceCloud(
                getAbcService().id(),
                getAbcService().slug(),
                getAbcdFolder().id(),
                getState().getCloudId(),
                getState().getDefaultFolderId()
        ));

        return getState()
                .withStatus(Status.SUCCEEDED);
    }

}
