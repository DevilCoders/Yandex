package ru.yandex.ci.storage.core.large;

import java.util.Objects;
import java.util.Optional;
import java.util.Set;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi;
import ru.yandex.ci.api.proto.Common.CommitId;
import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.core.proto.CiCoreProtoMappers;
import ru.yandex.ci.core.proto.ProtobufSerialization;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common.CheckTaskType;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_task.LargeTaskEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.util.CiJson;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Slf4j
@RequiredArgsConstructor
public class LargeFlowStartService {

    private static final Set<CheckTaskType> ACCEPT_CHECK_TASK_TYPES =
            Set.of(CheckTaskType.CTT_LARGE_TEST, CheckTaskType.CTT_NATIVE_BUILD);

    private final CiClient ciClient;
    private final StorageCoreCache<?> storageCache;
    private final CiStorageDb db;
    private final BazingaTaskManager bazingaTaskManager;

    public void startLargeFlow(LargeTaskEntity.Id largeTaskId) {
        log.info("Starting CI flow for {}", largeTaskId);
        new FlowProcessorImpl(largeTaskId).process();
    }

    private class FlowProcessorImpl {
        private final LargeTaskEntity.Id largeTaskId;
        private final LargeTaskEntity largeTask;
        private final CheckEntity check;

        private FlowProcessorImpl(LargeTaskEntity.Id largeTaskId) {
            this.largeTaskId = largeTaskId;
            this.largeTask = storageCache.largeTasks().getFreshOrThrow(largeTaskId);
            this.check = storageCache.checks().getFreshOrThrow(largeTaskId.getIterationId().getCheckId());
        }

        public void process() {
            startFlowIfRequired();
            markDiscoveredCommit();
        }

        private void startFlowIfRequired() {
            if (largeTask.getLaunchNumber() != null) {
                log.info("Large task is started already: {}", largeTask.getLaunchNumber());
                return; // ---
            }
            var flowProcessId = CiCoreProtoMappers.toProtoFlowProcessId(largeTask.toCiProcessId());

            var rev = check.getRight();
            var startFlowRequestBuilder = FrontendOnCommitFlowLaunchApi.StartFlowRequest.newBuilder()
                    .setFlowProcessId(flowProcessId)
                    .setBranch(rev.getBranch())
                    .setRevision(CommitId.newBuilder().setCommitId(rev.getRevision()))
                    .setNotifyPullRequest(false);  // Do not notify PR

            Optional.ofNullable(largeTask.getConfigRevision())
                    .map(CiCoreProtoMappers::toProtoOrderedArcRevision)
                    .ifPresent(startFlowRequestBuilder::setConfigRevision);


            var requestBuilder = StorageApi.ExtendedStartFlowRequest.newBuilder()
                    .setRequest(startFlowRequestBuilder)
                    .setFlowVars(buildFlowVars());

            Optional.ofNullable(largeTask.getDelegatedConfig())
                    .map(CheckProtoMappers::toDelegatedConfig)
                    .ifPresent(requestBuilder::setDelegatedConfig);

            requestBuilder.setPostponed(
                    check.getType() == CheckOuterClass.CheckType.TRUNK_POST_COMMIT ||
                            check.getType() == CheckOuterClass.CheckType.BRANCH_POST_COMMIT);

            var request = requestBuilder.build();
            var response = ciClient.startFlow(request);

            // I don't think it's a good idea to keep record locked during flow launch
            // Just assume Bazinga will not fail and won't run same task more than once at the same time
            var largeTaskUpdate = largeTask.withLaunchNumber(response.getLaunch().getNumber());
            storageCache.modifyWithDbTx(modifiable ->
                    modifiable.largeTasks().writeThrough(largeTaskUpdate));
        }

        private void markDiscoveredCommit() {
            if (check.getType() != CheckOuterClass.CheckType.TRUNK_POST_COMMIT ||
                    !ACCEPT_CHECK_TASK_TYPES.contains(largeTaskId.getCheckTaskType())) {
                log.info("Skip marking commit as discovered, check type: {}, task type: {}",
                        check.getType(), largeTaskId.getCheckTaskType());
                return; // ---
            }

            var iterationId = largeTaskId.getIterationId();
            var iteration = storageCache.iterations().getFreshOrThrow(iterationId);

            if (iteration.getStartedBy() != null) {
                log.info("Skip manual start");
                return; // ---
            }

            var tasks = db.currentOrReadOnly(() ->
                    db.largeTasks().findAllLargeTasks(
                            iterationId.getCheckId(),
                            iterationId.getIterationType(),
                            ACCEPT_CHECK_TASK_TYPES));
            log.info("Found {} total large tasks for all supported types", tasks.size());

            Preconditions.checkState(!tasks.isEmpty());
            var notReady = tasks.stream()
                    .map(LargeTaskEntity::getLaunchNumber)
                    .filter(Objects::isNull)
                    .count();

            if (notReady == 0) {
                db.currentOrTx(() -> {
                    log.info("Schedule marking commit as discovered: {}", check.getRight().getRevision());
                    bazingaTaskManager.schedule(new MarkDiscoveredCommitTask(check.getId()));
                });
            } else {
                log.info("{} large tasks are still not ready", notReady);
            }
        }

        private String buildFlowVars() {
            var request = CheckProtoMappers.toProtoLargeTaskRequest(largeTaskId);
            var testInfo = CheckProtoMappers.toProtoTestId(largeTask);

            var flowVars = CiJson.mapper().createObjectNode();
            flowVars.set("request", ProtobufSerialization.serializeToJson(request));
            flowVars.set("testInfo", ProtobufSerialization.serializeToJson(testInfo));

            return flowVars.toString();
        }
    }

}
