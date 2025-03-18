package ru.yandex.ci.tms.test;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;
import java.util.function.Function;

import javax.annotation.Nullable;

import WoodflowCi.Woodflow;
import WoodflowCi.furniture_factory.FurnitureFactory;
import WoodflowCi.woodcutter.Woodcutter;
import com.google.common.base.Preconditions;
import com.google.gson.JsonObject;
import com.google.protobuf.InvalidProtocolBufferException;
import one.util.streamex.StreamEx;
import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.arcanum.ArcanumTestServer;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.common.bazinga.spring.S3BazingaLoggerTestConfig;
import ru.yandex.ci.common.bazinga.spring.S3LogStorageConfig;
import ru.yandex.ci.common.bazinga.spring.TestZkConfig;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.common.temporal.spring.TemporalTestConfig;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.abc.AbcServiceStub;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.ArcServiceStub;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.launch.FlowReference;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.logbroker.LogbrokerWriter;
import ru.yandex.ci.core.security.PermissionsService;
import ru.yandex.ci.core.spring.clients.PciExpressTestConfig;
import ru.yandex.ci.core.spring.clients.TaskletV2ClientTestConfig;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataHelper;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.core.timeline.Offset;
import ru.yandex.ci.core.timeline.TimelineItem;
import ru.yandex.ci.engine.EngineTester;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.discovery.DiscoveryServicePostCommits;
import ru.yandex.ci.engine.discovery.DiscoveryServicePullRequests;
import ru.yandex.ci.engine.event.Event;
import ru.yandex.ci.engine.event.LaunchMappers;
import ru.yandex.ci.engine.event.NoOpAsyncProducerWithStore;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.OnCommitLaunchService;
import ru.yandex.ci.engine.pcm.PCMService;
import ru.yandex.ci.engine.spring.EngineJobs;
import ru.yandex.ci.engine.spring.LogbrokerLaunchEventTestConfig;
import ru.yandex.ci.engine.spring.PermissionsConfig;
import ru.yandex.ci.engine.spring.TestServicesHelpersConfig;
import ru.yandex.ci.engine.spring.TestenvDegradationTestConfig;
import ru.yandex.ci.engine.spring.TrackerJobs;
import ru.yandex.ci.engine.spring.jobs.TrackerTestConfig;
import ru.yandex.ci.engine.spring.tasks.EngineTasksConfig;
import ru.yandex.ci.engine.test.schema.SimpleData;
import ru.yandex.ci.engine.timeline.TimelineService;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.DispatcherJobScheduler;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResource;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredSourceCodeObject;
import ru.yandex.ci.flow.engine.runtime.events.DisableJobManualSwitchEvent;
import ru.yandex.ci.flow.engine.runtime.events.TriggerEvent;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestQueries;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.spring.TestJobsConfig;
import ru.yandex.ci.flow.spring.tasks.FlowTasksBazingaConfig;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.ci.job.TaskletContext;
import ru.yandex.ci.tms.spring.EntireTestConfig;
import ru.yandex.ci.tms.spring.TestClientsConfig;
import ru.yandex.ci.tms.spring.bazinga.BazingaServiceConfig;
import ru.yandex.ci.tms.spring.tasks.AutoReleaseTaskConfig;
import ru.yandex.ci.tms.spring.temporal.CiTmsTemporalWorkerConfig;
import ru.yandex.ci.tms.test.woodflow.FurnitureFactoryStub;
import ru.yandex.ci.tms.test.woodflow.PicklockStub;
import ru.yandex.ci.tms.test.woodflow.SawmillStub;
import ru.yandex.ci.tms.test.woodflow.WoodcutterStub;
import ru.yandex.commune.bazinga.BazingaTaskManager;

import static org.assertj.core.api.Assertions.fail;

@ContextConfiguration(classes = {
        TemporalTestConfig.class,
        CiTmsTemporalWorkerConfig.class,
        EngineTasksConfig.class,
        FlowTasksBazingaConfig.class,
        BazingaServiceConfig.class,
        S3LogStorageConfig.class,
        LogbrokerLaunchEventTestConfig.class,
        PermissionsConfig.class,
        EngineJobs.class,
        TrackerJobs.class,
        AutoReleaseTaskConfig.class,

        S3BazingaLoggerTestConfig.class,
        TestenvDegradationTestConfig.class,
        TestClientsConfig.class,
        TestServicesHelpersConfig.class,
        TrackerTestConfig.class,
        TestZkConfig.class,
        EntireTestConfig.class,
        TestJobsConfig.class,
        PciExpressTestConfig.class,
        TaskletV2ClientTestConfig.class
})
public abstract class AbstractEntireTest extends YdbCiTestBase {
    protected static final Duration WAIT = Duration.ofMinutes(2);

    //Need for autocheck info handler
    @MockBean
    protected PCMService pcmService;

    @Autowired
    protected LaunchService launchService;

    @Autowired
    protected OnCommitLaunchService onCommitLaunchService;

    @Autowired
    protected DiscoveryServicePullRequests discoveryServicePullRequests;

    @Autowired
    protected DiscoveryServicePostCommits discoveryServicePostCommits;

    @Autowired
    protected ArcService arcService;

    @Autowired
    protected SandboxClient sandboxClient;

    @Autowired
    protected EngineTester engineTester;

    @Autowired
    protected FlowTestQueries flowTestQueries;

    @Autowired
    protected TimelineService timelineService;

    @Autowired
    protected BranchService branchService;

    @Autowired
    protected ArcanumClientImpl arcanumClient;

    @Autowired
    protected FlowStateService flowStateService;

    @Autowired
    protected LogbrokerWriter testQueue;

    @Autowired
    protected BazingaTaskManager bazingaTaskManager;

    @Autowired
    protected Clock clock;

    @Autowired
    protected DispatcherJobScheduler jobScheduler;

    @Autowired
    protected PermissionsService permissionsService;

    @Autowired
    protected AbcService abcService;

    @Autowired
    protected TaskletV2MetadataHelper taskletV2MetadataHelper;

    @Autowired
    protected ArcanumTestServer arcanumTestServer;

    protected ArcServiceStub arcServiceStub;
    protected SandboxClientTaskletExecutorStub sandboxClientStub;
    protected AbcServiceStub abcServiceStub;
    protected NoOpAsyncProducerWithStore noOpTestQueue;

    @BeforeEach
    public void setUp() {
        arcServiceStub = (ArcServiceStub) arcService;
        sandboxClientStub = (SandboxClientTaskletExecutorStub) sandboxClient;
        abcServiceStub = (AbcServiceStub) abcService;
        noOpTestQueue = (NoOpAsyncProducerWithStore) testQueue;

        arcServiceStub.resetAndInitTestData();
        sandboxClientStub.reset();

        // Необходимо загрузить все тасклеты, в противном случае a.yaml будет невалидным
        this.uploadTasklets();
        this.uploadTaskletsV2();

        noOpTestQueue.getQueue().clear();
        jobScheduler.flush();

        arcanumTestServer.reset();
        arcanumTestServer.mockSetMergeRequirement();
        arcanumTestServer.mockSetMergeRequirementStatus();
    }

    protected void enableTemporal() {
        db.tx(() -> db.keyValue().setValue(
                DispatcherJobScheduler.KV_NAMESPACE, DispatcherJobScheduler.DEFAULT_KEY, true
        ));
    }

    protected void ensureOnlyTemporal() {
        Assertions.assertThat(jobScheduler.getBazingaLaunchCount()).isEqualTo(0);
    }

    protected List<String> getFurniture(StoredResourceContainer resources) {
        return getResources(resources, JobResourceType.ofDescriptor(Woodflow.Furniture.getDescriptor()),
                o -> o.getAsJsonObject("data").get("description").getAsString());
    }

    protected List<String> getBoards(StoredResourceContainer resources) {
        return getResources(resources, JobResourceType.ofDescriptor(Woodflow.Board.getDescriptor()),
                o -> "%s[%s]".formatted(
                        o.getAsJsonObject("data").get("producer").getAsString(),
                        o.getAsJsonObject("data").getAsJsonObject("source").get("name").getAsString()));
    }

    protected List<String> getResources(StoredResourceContainer resources, JobResourceType resourceType,
                                        Function<JsonObject, String> extract) {
        return StreamEx.of(resources.getResources())
                .filterBy(StoredResource::getResourceType, resourceType)
                .map(StoredSourceCodeObject::getObject)
                .map(extract)
                .sorted()
                .toList();
    }

    protected List<TimelineItem> getTimeline(CiProcessId processId) {
        return getTimeline(processId, ArcBranch.trunk());
    }

    protected List<TimelineItem> getTimeline(CiProcessId processId, ArcBranch branch) {
        return timelineService.getTimeline(processId, branch, Offset.EMPTY, -1);
    }

    protected void uploadTasklets() {
        sandboxClientStub.uploadTasklet(TaskletResources.WOODCUTTER, new WoodcutterStub());
        sandboxClientStub.uploadTasklet(TaskletResources.SAWMILL, new SawmillStub());
        sandboxClientStub.uploadTasklet(TaskletResources.FURNITURE, new FurnitureFactoryStub());
        sandboxClientStub.uploadTasklet(TaskletResources.PICKLOCK, new PicklockStub());

        // на четвертой ревизии версия тасклета меняется
        sandboxClientStub.uploadTasklet(TaskletResources.SAWMILL_UPDATED, new SawmillStub());
    }

    protected void uploadTaskletsV2() {
        taskletV2MetadataHelper.registerExecutor(
                TaskletV2Resources.WOODCUTTER,
                Woodcutter.Input.class,
                Woodcutter.Output.class,
                new WoodcutterStub()::execute
        );

        taskletV2MetadataHelper.registerExecutor(
                TaskletV2Resources.FURNITURE,
                FurnitureFactory.Input.class,
                FurnitureFactory.Output.class,
                new FurnitureFactoryStub()::execute
        );
        taskletV2MetadataHelper.registerExecutor(
                TaskletV2Resources.SIMPLE,
                SimpleData.class,
                SimpleData.class,
                new SimpleAction()
        );
        taskletV2MetadataHelper.registerExecutor(
                TaskletV2Resources.SIMPLE_INVALID,
                SimpleData.class,
                SimpleData.class,
                new SimpleAction()
        );

    }

    protected Launch launch(
            CiProcessId processId,
            OrderedArcRevision revision
    ) {
        return launch(processId, revision, builder -> {
        });
    }

    protected Launch launch(
            CiProcessId processId,
            OrderedArcRevision revision,
            LaunchPullRequestInfo pullRequestInfo
    ) {
        return launch(processId, revision,
                builder -> builder
                        .launchPullRequestInfo(pullRequestInfo)
                        .notifyPullRequest(pullRequestInfo != null)
        );
    }

    protected Launch launch(
            CiProcessId processId,
            OrderedArcRevision revision,
            JsonObject flowVars
    ) {
        return launch(processId, revision,
                builder -> builder
                        .flowVars(flowVars)
        );
    }

    protected Launch launch(
            CiProcessId processId,
            OrderedArcRevision revision,
            String customFlowId,
            Common.FlowType customFlowType,
            @Nullable LaunchId rollbackUsingLaunch
    ) {
        return launch(processId, revision,
                builder -> builder
                        .flowReference(FlowReference.of(customFlowId, customFlowType))
                        .rollbackUsingLaunch(rollbackUsingLaunch)
        );
    }

    protected Launch launch(
            CiProcessId processId,
            OrderedArcRevision revision,
            Consumer<LaunchService.LaunchParameters.Builder> configuration
    ) {
        ConfigBundle bundle = launchService.getLastValidConfig(processId, revision.getBranch());
        var builder = LaunchService.LaunchParameters.builder()
                .processId(processId)
                .launchType(Launch.Type.USER)
                .bundle(bundle)
                .triggeredBy(TestData.CI_USER)
                .revision(revision)
                .selectedBranch(revision.getBranch());
        configuration.accept(builder);
        return launchService.createAndStartLaunch(builder.build());
    }

    protected void processCommits(ArcCommit... commits) {
        processCommits(ArcBranch.trunk(), commits);
    }

    protected void processCommits(ArcBranch trunk, ArcCommit... commits) {
        for (var commit : commits) {
            upsertCommit(commit);
            discoveryServicePostCommits.processPostCommit(trunk, commit.getRevision(), false);
        }
    }

    protected void upsertCommit(ArcCommit commit) {
        db.currentOrTx(() -> db.arcCommit().save(commit));
    }

    protected void expectEvents(Launch launch, Function<Event, List<Event>> expectEvents) throws InterruptedException,
            InvalidProtocolBufferException {
        var expectations = expectEvents.apply(LaunchMappers.launchContext(launch)
                .setFlowLaunchId("")
                .build());

        var toCheck = new HashSet<>(expectations);
        var actualEvents = new ArrayList<Event>(expectations.size());
        Event previous = null;
        while (!toCheck.isEmpty()) {
            var actual = fetchNextEvent(launch);
            if (actual.isPresent()) {
                var actualEvent = actual.get();
                actualEvents.add(actualEvent);
                if (actualEvent.equals(previous)) {
                    // skip duplicate event
                    continue; // ---
                }
                previous = actualEvent;
                if (toCheck.remove(actualEvent)) {
                    continue; // ---
                }
            }

            var expectedTypes = expectations.stream().map(Event::getEventType).toList();
            var actualTypes = actualEvents.stream().map(Event::getEventType).toList();
            var actualType = actual.map(e -> e.getEventType().toString()).orElse("<no event>");

            fail("""
                            ------
                            excepted events:
                            %s
                            ------
                            actual:
                            %s
                            ------
                            not expected:
                            %s

                            Detailed event dumps you can find above
                            expected: %s, actual: %s, not expected: %s
                            """,
                    expectations, actualEvents, actual.map(Objects::toString).orElse("<no event>"),
                    expectedTypes, actualTypes, actualType
            );
            break;
        }
    }

    protected Optional<Event> fetchNextEvent(Launch launch)
            throws InterruptedException, InvalidProtocolBufferException {
        while (true) {
            var content = noOpTestQueue.getQueue().poll(15, TimeUnit.SECONDS);
            if (content == null) {
                return Optional.empty(); // ---
            }
            var event = Event.parseFrom(content.getData());
            if (event.getLaunchId().equals(launch.getId().getProcessId())) {
                Preconditions.checkArgument(event.getTimestampMillis() > 0);
                return Optional.of(event.toBuilder().setTimestampMillis(0).build());
            }
        }
    }

    protected Instant disableManualSwitch(Launch launch, String jobId) throws InterruptedException {
        engineTester.waitLaunchFlowStarted(launch.getLaunchId(), WAIT);

        var now = Instant.now();
        flowStateService.recalc(
                FlowLaunchId.of(launch.getLaunchId()),
                new DisableJobManualSwitchEvent(jobId, "username", now)
        );
        return now;
    }

    protected void launchJob(Launch launch, String jobId, String username) {
        flowStateService.recalc(
                FlowLaunchId.of(launch.getLaunchId()),
                new TriggerEvent(jobId, username, false)
        );
    }

    protected TaskletContext updateContext(TaskletContext context, Launch launch, FlowLaunchEntity flowLaunch) {
        var flowStarted = flowLaunch.getCreatedDate();
        var jobId = context.getJobInstanceId().getJobId();
        var jobStarted = flowLaunch.getJobState(jobId).getLastLaunch().getFirstStatusChange().getDate();

        var builder = context.toBuilder();
        builder.getJobInstanceIdBuilder().setFlowLaunchId(flowLaunch.getIdString());
        builder.setTitle(launch.getTitle());
        builder.setJobStartedAt(ProtoConverter.convert(jobStarted));
        builder.setFlowStartedAt(ProtoConverter.convert(flowStarted));
        return builder.build();
    }
}
