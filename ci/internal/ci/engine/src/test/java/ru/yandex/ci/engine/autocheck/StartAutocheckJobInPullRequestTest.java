package ru.yandex.ci.engine.autocheck;

import java.io.IOException;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import io.github.benas.randombeans.api.EnhancedRandom;
import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.stub.StreamObserver;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.junit.jupiter.api.extension.RegisterExtension;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.client.abc.AbcClient;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.client.arcanum.ArcanumTestServer;
import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.client.testenv.TestenvClient;
import ru.yandex.ci.client.yav.YavClient;
import ru.yandex.ci.common.bazinga.spring.TestZkConfig;
import ru.yandex.ci.common.grpc.GrpcCleanupExtension;
import ru.yandex.ci.common.grpc.GrpcClientPropertiesStub;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.ArcServiceStub;
import ru.yandex.ci.core.autocheck.FlowVars;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.pr.PullRequestVcsInfo;
import ru.yandex.ci.core.resolver.PropertiesSubstitutor;
import ru.yandex.ci.core.spring.clients.ArcClientTestConfig;
import ru.yandex.ci.core.spring.clients.TaskletV2ClientTestConfig;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.StartAutocheckJob;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.ci.engine.pcm.PCMService;
import ru.yandex.ci.engine.spring.AutocheckConfig;
import ru.yandex.ci.engine.spring.clients.StaffClientTestConfig;
import ru.yandex.ci.engine.spring.clients.XivaClientTestConfig;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobContext;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.ci.storage.api.StorageApi.RegisterCheckIterationRequest;
import ru.yandex.ci.storage.api.StorageApi.RegisterCheckRequest;
import ru.yandex.ci.storage.api.StorageApi.RegisterCheckResponse;
import ru.yandex.ci.storage.api.StorageApi.RegisterTaskRequest;
import ru.yandex.ci.storage.api.StorageApiServiceGrpc;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.test.random.TestRandomUtils;
import ru.yandex.ci.util.ResourceUtils;
import ru.yandex.ci.util.gson.CiGson;
import ru.yandex.commune.bazinga.ydb.storage.BazingaStorageDb;
import ru.yandex.passport.tvmauth.TvmClient;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.registerFormatterForType;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doReturn;
import static org.mockserver.model.HttpRequest.request;
import static org.springframework.http.HttpMethod.GET;
import static ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePullRequests.TRUNK_PRECOMMIT_PROCESS_ID;

@ContextConfiguration(classes = {
        ArcClientTestConfig.class,
        AutocheckConfig.class,
        StartAutocheckJobInPullRequestTest.Config.class,
        StaffClientTestConfig.class,
        XivaClientTestConfig.class,
        TaskletV2ClientTestConfig.class,
        TestZkConfig.class,
})
@MockBean(DiscoveryProgressService.class)
@MockBean(AbcClient.class)
@MockBean(PCMService.class)
@MockBean(SandboxClient.class)
@MockBean(BazingaStorageDb.class)   // we don't test scheduling bazinga tasks
@MockBean(YavClient.class)
@MockBean(TestenvClient.class)
@MockBean(TvmClient.class)
@Timeout(value = 10)
public class StartAutocheckJobInPullRequestTest extends CommonYdbTestBase {

    private static final String STORAGE_IN_PROCESS_SERVER = InProcessServerBuilder.generateName();

    private final Gson gson = CiGson.instance();

    private static final long SEED = -132185L;
    private static final EnhancedRandom RANDOM = TestRandomUtils.enhancedRandom(SEED);

    @RegisterExtension
    public GrpcCleanupExtension grpcCleanup = new GrpcCleanupExtension();
    private StorageApiServiceImpl storageApiServiceImpl;

    @Autowired
    StartAutocheckJob startAutocheckJob;

    @Autowired
    ArcanumTestServer arcanumTestServer;

    @Autowired
    TvmClient tvmClient;

    @Autowired
    StorageApiClient storageApiClient;


    @BeforeEach
    void setUp() throws IOException {
        storageApiServiceImpl = new StorageApiServiceImpl();
        grpcCleanup.register(InProcessServerBuilder
                .forName(STORAGE_IN_PROCESS_SERVER)
                .directExecutor()
                .addService(storageApiServiceImpl)
                .build()
                .start());

        doReturn("service-ticket").when(tvmClient).getServiceTicketFor(anyInt());
        arcanumTestServer.reset();
        storageApiServiceImpl.reset();
    }

    @Test
    void execute() throws Exception {
        arcanumTestServer.mockGetReviewRequestData(1998163, "autocheck/arcanum-response-1998163.json");
        mockArcanumRegistrationResponses();

        var flowVars = new JsonObject();
        flowVars.addProperty(FlowVars.IS_PRECOMMIT, true);

        startAutocheckJob.execute(createJobContext(TRUNK_PRECOMMIT_PROCESS_ID, flowVars));

        verifyStorageRegisterCheckRequest("autocheck/pullrequests/storage.registerCheckRequest.json");
        verifyStorageRegisterIterationRequest();
        verifyStorageRegisterTaskRequest();
    }

    @Test
    void execute_withStressTestInFlowVars() throws Exception {
        arcanumTestServer.mockGetReviewRequestData(1998163, "autocheck/arcanum-response-1998163.json");
        mockArcanumRegistrationResponses();

        var flowVars = new JsonObject();
        flowVars.addProperty(FlowVars.IS_PRECOMMIT, true);
        flowVars.addProperty(FlowVars.IS_STRESS_TEST, true);

        startAutocheckJob.execute(createJobContext(TRUNK_PRECOMMIT_PROCESS_ID, flowVars));

        verifyStorageRegisterCheckRequest(
                "autocheck/pullrequests/storage.registerCheckRequest-withStressTestInFlowVars.json");
        verifyStorageRegisterIterationRequest();
        verifyStorageRegisterTaskRequest();

        arcanumTestServer.clear(request("/api/v1/review-requests/1998163").withMethod(GET.name()));
        arcanumTestServer.verifyZeroInteractions();
    }

    private void mockArcanumRegistrationResponses() {
        arcanumTestServer.mockRegisterCiCheck();
        arcanumTestServer.mockSetMergeRequirementStatus();
    }

    private void verifyStorageRegisterCheckRequest(String expectedRegisterCheckRequestFilepath) {
        assertThat(storageApiServiceImpl.checks).isEqualTo(Set.of(
                parseJsonFromFile(expectedRegisterCheckRequestFilepath, RegisterCheckRequest.class)
        ));
    }

    private void verifyStorageRegisterIterationRequest() {
        assertThat(storageApiServiceImpl.iterations).isEqualTo(Set.of(
                parseJsonFromFile(
                        "autocheck/pullrequests/storage.registerIterationRequest.json",
                        RegisterCheckIterationRequest[].class
                )
        ));
    }

    private void verifyStorageRegisterTaskRequest() {
        assertThat(storageApiServiceImpl.tasks).isEqualTo(Set.of(
                parseJsonFromFile(
                        "autocheck/pullrequests/storage.registerTasksRequest.json",
                        RegisterTaskRequest[].class
                )
        ));
    }

    private <T> T parseJsonFromFile(String filePath, Class<T> clazz) {
        return parseJson(ResourceUtils.textResource(filePath), clazz);
    }

    private <T> T parseJson(String json, Class<T> clazz) {
        if (clazz.isArray()) {
            registerFormatterForType(clazz.getComponentType(), gson::toJson);
        } else {
            registerFormatterForType(clazz, gson::toJson);
        }
        return gson.fromJson(json, clazz);
    }

    private static TestJobContext createJobContext(CiProcessId processId, JsonObject flowVars) {
        var pullRequestVcsInfo = new PullRequestVcsInfo(
                TestData.DS2_REVISION,
                TestData.TRUNK_R2.toRevision(),
                ArcBranch.trunk(),
                TestData.REVISION,
                TestData.USER_BRANCH
        );

        var pullRequestInfo = new LaunchPullRequestInfo(
                1998163, 4285430, TestData.CI_USER,
                "Some summary", "Some description",
                ArcanumMergeRequirementId.of("x", "y"),
                pullRequestVcsInfo, List.of("CI-1"), List.of("label1"), null
        );

        var flowLaunchEntity = RANDOM.nextObject(FlowLaunchEntity.class).toBuilder()
                .processId(processId.asString())
                .launchNumber(42)
                .id(FlowLaunchId.of("flow-id"))
                .flowInfo(
                        RANDOM.nextObject(LaunchFlowInfo.class).toBuilder()
                                .flowId(FlowFullId.of(processId.getPath(), "autocheck"))
                                .flowVars(JobResource.optional(
                                        JobResourceType.of(PropertiesSubstitutor.FLOW_VARS_KEY), flowVars
                                ))
                                .build()
                )
                .vcsInfo(
                        LaunchVcsInfo.builder()
                                .previousRevision(TestData.TRUNK_R2)
                                .revision(TestData.DIFF_SET_1.getOrderedMergeRevision())
                                .pullRequestInfo(pullRequestInfo)
                                .build()
                )
                .jobs(Map.of())
                .build();

        return new TestJobContext(flowLaunchEntity);
    }


    @Configuration
    static class Config {

        @Bean
        public ArcService arcService() {
            return new ArcServiceStub(
                    "autocheck/pullrequests/test-repo",
                    TestData.TRUNK_COMMIT_2,
                    TestData.DS1_COMMIT.withParent(TestData.TRUNK_COMMIT_2)
            );
        }

        @Bean
        ArcanumTestServer arcanumTestServer() {
            return new ArcanumTestServer();
        }

        @Bean
        HttpClientProperties httpClientProperties(ArcanumTestServer arcanumTestServer) {
            return HttpClientPropertiesStub.of(arcanumTestServer.getServer());
        }

        @Bean
        StorageApiClient storageApiClient(TvmClient tvmClient) {
            return StorageApiClient.create(GrpcClientPropertiesStub.of(STORAGE_IN_PROCESS_SERVER));
        }

        @Bean
        public StartAutocheckJob startAutocheckJob(
                AutocheckService autocheckService,
                AutocheckBlacklistService autocheckBlacklistService,
                CiMainDb db,
                UrlService urlService
        ) {
            return new StartAutocheckJob(
                    autocheckService,
                    autocheckBlacklistService,
                    db,
                    urlService
            );
        }

    }

    private static class StorageApiServiceImpl extends StorageApiServiceGrpc.StorageApiServiceImplBase {

        Set<RegisterCheckRequest> checks = new LinkedHashSet<>();
        Set<RegisterCheckIterationRequest> iterations = new LinkedHashSet<>();
        Set<RegisterTaskRequest> tasks = new LinkedHashSet<>();

        public void reset() {
            checks.clear();
            iterations.clear();
            tasks.clear();
        }

        @Override
        public void registerCheck(RegisterCheckRequest request,
                                  StreamObserver<RegisterCheckResponse> responseObserver) {
            checks.add(request);
            responseObserver.onNext(RegisterCheckResponse.newBuilder()
                    .setCheck(CheckOuterClass.Check.newBuilder()
                            .setDiffSetId(request.getDiffSetId())
                            .setOwner(request.getOwner())
                            .addAllTags(request.getTagsList())
                            .setInfo(request.getInfo())
                            .build()
                    )
                    .build());
            responseObserver.onCompleted();
        }

        @Override
        public void registerCheckIteration(RegisterCheckIterationRequest request,
                                           StreamObserver<CheckIteration.Iteration> responseObserver) {
            iterations.add(request);
            responseObserver.onNext(CheckIteration.Iteration.newBuilder()
                    .setId(CheckIteration.IterationId.newBuilder()
                            .setCheckId(request.getCheckId())
                            .setCheckType(request.getCheckType())
                            .build()
                    )
                    .build());
            responseObserver.onCompleted();
        }

        @Override
        public void registerTask(RegisterTaskRequest request,
                                 StreamObserver<CheckTaskOuterClass.CheckTask> responseObserver) {
            tasks.add(request);
            responseObserver.onNext(CheckTaskOuterClass.CheckTask.newBuilder()
                    .setId(CheckTaskOuterClass.FullTaskId.newBuilder()
                            .setTaskId(request.getTaskId())
                            .build())
                    .build());
            responseObserver.onCompleted();
        }

    }

}
