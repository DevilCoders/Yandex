package ru.yandex.ci.engine.launch.auto;

import java.time.Clock;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.function.Consumer;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import com.google.protobuf.ByteString;
import com.google.protobuf.InvalidProtocolBufferException;
import lombok.ToString;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.PostponeLaunch;
import ru.yandex.ci.core.launch.PostponeLaunch.PostponeStatus;
import ru.yandex.ci.core.proto.ProtobufSerialization;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.api.StorageApi.GetLargeTaskRequest;
import ru.yandex.ci.storage.api.StorageApi.SendMessagesRequest;
import ru.yandex.ci.storage.core.Actions;
import ru.yandex.ci.storage.core.Actions.TestType;
import ru.yandex.ci.storage.core.Actions.TestTypeSizeFinished;
import ru.yandex.ci.storage.core.Actions.TestTypeSizeFinished.Size;
import ru.yandex.ci.storage.core.CheckTaskOuterClass.FullTaskId;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.MainStreamMessages.MainStreamMessage;
import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.TaskMessages.AutocheckTestResult;
import ru.yandex.ci.storage.core.TaskMessages.AutocheckTestResults;
import ru.yandex.ci.storage.core.TaskMessages.TaskMessage;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@Slf4j
public class LargePostCommitWriterTask extends AbstractOnetimeTask<LargePostCommitWriterTask.Params> {

    private StorageApiClient storageApiClient;
    private UrlService urlService;
    private CiDb db;
    private Clock clock;

    public LargePostCommitWriterTask(
            StorageApiClient storageApiClient,
            UrlService urlService,
            CiDb db,
            Clock clock
    ) {
        super(LargePostCommitWriterTask.Params.class);
        this.storageApiClient = storageApiClient;
        this.urlService = urlService;
        this.db = db;
        this.clock = clock;
    }

    public LargePostCommitWriterTask(PostponeLaunch.Id id) {
        super(new Params(id));
    }

    @Override
    protected void execute(Params params, ExecutionContext context) throws Exception {
        sendToStorage(params.toId());
    }

    public void sendToStorage(PostponeLaunch.Id postponeId) throws Exception {
        log.info("Processing postpone launch {}", postponeId);

        // Make sure Storage receives skip or failure for this large test
        var launches = db.currentOrReadOnly(() -> {
            var postponeLaunch = db.postponeLaunches().get(postponeId);
            var postponeStatus = postponeLaunch.getStatus();
            if (!acceptPostpone(postponeStatus)) {
                log.info("Ignore postpone processing, status is ignored: {}", postponeStatus);
                return null; // ---
            }
            var launch = db.launches().get(postponeLaunch.toLaunchId());
            return LoadedLaunches.of(postponeLaunch, launch);
        });

        if (launches == null) {
            log.warn("Postpone launch status may be changed: {}", postponeId);
            return; // ---
        }

        var largeTask = storageApiClient.getLargeTask(GetLargeTaskRequest.newBuilder()
                .setId(toLargeTaskId(launches.launch))
                .build());

        var request = SendMessagesRequest.newBuilder();
        for (var job : largeTask.getJobsList()) {
            request.setCheckType(job.getCheckType()); // All check types are the same, no need to check it here

            request.addMessages(toMessage(launches, job));
            request.addAllMessages(toCompletion(job.getId()));
        }
        storageApiClient.sendMessages(request.build());
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(5);
    }

    private MainStreamMessage toMessage(LoadedLaunches launches, StorageApi.LargeTestJob job) {
        var testInfo = job.getTestInfoSource();

        var testResult = AutocheckTestResult.newBuilder();
        testResult.getIdBuilder()
                .setSuiteHid(testInfo.getSuiteHid())
                .setToolchain(testInfo.getToolchain())
                .setHid(testInfo.getSuiteHid());

        testResult.setResultType(Common.ResultType.RT_TEST_SUITE_LARGE);

        var status = launches.postponeLaunch.getStatus();
        if (status == PostponeStatus.SKIPPED) {
            testResult.setTestStatus(Common.TestStatus.TS_NOT_LAUNCHED);
            testResult.setSnippet(ByteString.copyFromUtf8("Large test is skipped"));
        } else if (status == PostponeStatus.FAILURE) {
            testResult.setTestStatus(Common.TestStatus.TS_INTERNAL);
            testResult.setSnippet(ByteString.copyFromUtf8("Unable to execute Large test flow"));
            var link = urlService.toLaunch(launches.launch);
            if (link != null) {
                testResult.getLinksBuilder().putLink("flow",
                        TaskMessages.Links.Link.newBuilder()
                                .addLink(link)
                                .build());
            }
        } else {
            throw new IllegalStateException("Internal error. Unsupported postpone status: " + status);
        }
        testResult.setUid("ci-" + UUID.randomUUID());
        testResult.setPath(job.getNativeTarget().isEmpty()
                ? job.getTarget()
                : job.getNativeTarget());
        testResult.setName(testInfo.getSuiteName());
        testResult.addAllTags(testInfo.getTagsList());
        testResult.setOldId(testInfo.getSuiteId());
        testResult.setOldSuiteId(testInfo.getSuiteId());

        return MainStreamMessage.newBuilder()
                .setTaskMessage(TaskMessage.newBuilder()
                        .setFullTaskId(job.getId())
                        .setAutocheckTestResults(AutocheckTestResults.newBuilder()
                                .addResults(testResult)
                        )
                )
                .build();
    }

    private List<MainStreamMessage> toCompletion(FullTaskId taskId) {
        var result = new ArrayList<MainStreamMessage>();
        result.add(toTestTypeSizeFinished(taskId, Size.LARGE));
        result.add(toTestTypeFinished(taskId, TestType.TEST));
        result.add(toFinish(taskId));
        return result;
    }

    private MainStreamMessage toTestTypeFinished(FullTaskId taskId, TestType testType) {
        return wrap(taskId, builder ->
                builder.setTestTypeFinished(Actions.TestTypeFinished.newBuilder()
                        .setTestType(testType)
                        .setTimestamp(ProtoMappers.toProtoTimestamp(clock.instant()))
                )
        );
    }

    private MainStreamMessage toTestTypeSizeFinished(FullTaskId taskId, Size size) {
        return wrap(taskId, builder ->
                builder.setTestTypeSizeFinished(
                        TestTypeSizeFinished.newBuilder()
                                .setTestType(TestType.TEST)
                                .setTimestamp(ProtoMappers.toProtoTimestamp(clock.instant()))
                                .setSize(size)
                )
        );
    }

    private MainStreamMessage toFinish(FullTaskId taskId) {
        return wrap(taskId, builder ->
                builder.setFinished(Actions.Finished.newBuilder()
                        .setTimestamp(ProtoMappers.toProtoTimestamp(clock.instant()))
                )
        );
    }

    private static MainStreamMessage wrap(
            FullTaskId taskId,
            Consumer<TaskMessage.Builder> taskBuilder
    ) {
        var builder = TaskMessage.newBuilder()
                .setFullTaskId(taskId);
        taskBuilder.accept(builder);
        return MainStreamMessage.newBuilder()
                .setTaskMessage(builder)
                .build();
    }

    static StorageApi.LargeTaskId toLargeTaskId(Launch launch) throws InvalidProtocolBufferException {
        var flowVars = launch.getFlowInfo().getFlowVars();
        Preconditions.checkState(flowVars != null,
                "Unable to load null flowVars from launch %s", launch.getLaunchId());

        var request = ProtobufSerialization.deserializeFromGson(
                flowVars.getData().getAsJsonObject("request"),
                GetLargeTaskRequest.newBuilder()
        );
        if (!request.getId().getIterationId().getCheckId().isEmpty()) {
            return request.getId();
        } else {
            return StorageApi.LargeTaskId.newBuilder()
                    .setIterationId(request.getIterationId())
                    .setCheckTaskType(request.getCheckTaskType())
                    .setIndex(request.getIndex())
                    .build();
        }
    }

    static boolean acceptPostpone(PostponeStatus postponeStatus) {
        return postponeStatus == PostponeStatus.SKIPPED ||
                postponeStatus == PostponeStatus.FAILURE;
    }

    @ToString
    @BenderBindAllFields
    public static class Params {
        private final String processId;
        private final String branch;
        private final long svnRevision;

        public Params(PostponeLaunch.Id id) {
            this.processId = id.getProcessId();
            this.branch = id.getBranch();
            this.svnRevision = id.getSvnRevision();
        }

        public PostponeLaunch.Id toId() {
            return PostponeLaunch.Id.of(processId, branch, svnRevision);
        }
    }

    @Value(staticConstructor = "of")
    private static class LoadedLaunches {
        @Nonnull
        PostponeLaunch postponeLaunch;
        @Nonnull
        Launch launch;
    }

}
