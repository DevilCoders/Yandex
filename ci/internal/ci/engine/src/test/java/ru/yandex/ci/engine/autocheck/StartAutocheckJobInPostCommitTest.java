package ru.yandex.ci.engine.autocheck;

import java.io.IOException;
import java.time.Instant;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Stream;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import io.github.benas.randombeans.api.EnhancedRandom;
import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.stub.StreamObserver;
import io.netty.handler.codec.http.HttpMethod;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.junit.jupiter.api.extension.RegisterExtension;
import org.mockserver.client.MockServerClient;
import org.mockserver.integration.ClientAndServer;
import org.mockserver.model.HttpStatusCode;
import org.mockserver.model.JsonBody;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.client.abc.AbcClient;
import ru.yandex.ci.client.abc.AbcTableClient;
import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.base.http.HttpClientPropertiesStub;
import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.staff.StaffClient;
import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.client.testenv.TestenvClient;
import ru.yandex.ci.client.yav.YavClient;
import ru.yandex.ci.common.bazinga.spring.TestZkConfig;
import ru.yandex.ci.common.grpc.GrpcCleanupExtension;
import ru.yandex.ci.common.grpc.GrpcClientPropertiesStub;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.ArcServiceStub;
import ru.yandex.ci.core.autocheck.AutocheckConstants.PostCommits;
import ru.yandex.ci.core.autocheck.FlowVars;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.resolver.PropertiesSubstitutor;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.core.spring.clients.TaskletV2ClientTestConfig;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.autocheck.jobs.autocheck.StartAutocheckJob;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.ci.engine.pcm.PCMService;
import ru.yandex.ci.engine.spring.AutocheckConfig;
import ru.yandex.ci.engine.spring.clients.XivaClientTestConfig;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobContext;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.api.StorageApiServiceGrpc;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.test.clock.OverridableClock;
import ru.yandex.ci.test.random.TestRandomUtils;
import ru.yandex.ci.util.ResourceUtils;
import ru.yandex.ci.util.gson.CiGson;
import ru.yandex.commune.bazinga.ydb.storage.BazingaStorageDb;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.registerFormatterForType;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;
import static ru.yandex.ci.engine.autocheck.AutocheckBootstrapServicePostCommits.TRUNK_POSTCOMMIT_PROCESS_ID;

@Slf4j
@ContextConfiguration(classes = {
        AutocheckConfig.class,
        StartAutocheckJobInPostCommitTest.Config.class,
        XivaClientTestConfig.class,
        TaskletV2ClientTestConfig.class,
        TestZkConfig.class,
})
@MockBean(DiscoveryProgressService.class)
@MockBean(AbcClient.class)
@MockBean(AbcTableClient.class)
@MockBean(PCMService.class)
@MockBean(BazingaStorageDb.class)   // we don't test scheduling bazinga tasks
@MockBean(YavClient.class)
@MockBean(ArcanumClientImpl.class)
@MockBean(SandboxClient.class)
@MockBean(TestenvClient.class)
@Timeout(value = 10)
public class StartAutocheckJobInPostCommitTest extends CommonYdbTestBase {

    private static final String STORAGE_IN_PROCESS_SERVER = InProcessServerBuilder.generateName();

    private final Gson gson = CiGson.instance();
    private final Gson prettyGson = new GsonBuilder().setPrettyPrinting().create();

    private static final long SEED = -1321957185L;
    private static final EnhancedRandom RANDOM = TestRandomUtils.enhancedRandom(SEED);
    private static final String STORAGE_CHECK_ID = "100500";

    @RegisterExtension
    public GrpcCleanupExtension grpcCleanup = new GrpcCleanupExtension();
    private StorageApiServiceImpl storageApiServiceImpl;

    @Autowired
    StartAutocheckJob startAutocheckJob;

    @Autowired
    StorageApiClient storageApiClient;

    @SpyBean
    ArcService arcService;

    @Autowired
    ArcanumClientImpl arcanumClient;

    @Autowired
    OverridableClock clock;

    @Autowired
    ClientAndServer staffServer;

    @Autowired
    AutocheckBlacklistService autocheckBlacklistService;

    @BeforeEach
    void setUp() throws IOException {
        storageApiServiceImpl = new StorageApiServiceImpl();
        grpcCleanup.register(InProcessServerBuilder
                .forName(StartAutocheckJobInPostCommitTest.STORAGE_IN_PROCESS_SERVER)
                .directExecutor()
                .addService(storageApiServiceImpl)
                .build()
                .start());

        storageApiServiceImpl.reset();
        List.of(staffServer).forEach(MockServerClient::reset);
        autocheckBlacklistService.resetCache();
        doReturn(TestData.TRUNK_COMMIT_3.getRevision())
                .when(arcService).getLastRevisionInBranch(eq(ArcBranch.trunk()));
    }

    @Test
    void execute() throws Exception {
        var flowVars = new JsonObject();
        flowVars.addProperty(FlowVars.IS_PRECOMMIT, false);

        mockStaffResponses(false);

        db.currentOrTx(() ->
                db.keyValue().setValue(PostCommits.NAMESPACE, PostCommits.ENABLED, true)
        );

        clock.setTime(Instant.ofEpochSecond(1645005431));
        var jobContext = createJobContext(TRUNK_POSTCOMMIT_PROCESS_ID, flowVars);
        startAutocheckJob.execute(jobContext);

        verifyStorageRegisterCheckRequest();
        verifyStorageRegisterIterationRequest();
        verifyStorageRegisterTaskRequest();
        verifyNoInteractions(arcanumClient);

        verifyProducedResourcesData(jobContext);
    }

    @Test
    void execute_whenCommitIsIgnored() throws Exception {
        var flowVars = new JsonObject();
        flowVars.addProperty(FlowVars.IS_PRECOMMIT, false);

        mockStaffResponses(true);

        db.currentOrTx(() ->
                db.keyValue().setValue(PostCommits.NAMESPACE, PostCommits.ENABLED, true)
        );

        clock.setTime(Instant.ofEpochSecond(1645005431));
        var jobContext = createJobContext(TRUNK_POSTCOMMIT_PROCESS_ID, flowVars);
        startAutocheckJob.execute(jobContext);

        verifyNoChecksRegistered();
        verifyNoInteractions(arcanumClient);

        log.info("data: {}", prettyGson.toJson(jobContext.resources().getProducedResources().stream()
                .map(Resource::getData)
                .toList()));
    }

    private void mockStaffResponses(boolean isRobot) {
        Stream.of(TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_2)
                .map(ArcCommit::getAuthor)
                .forEach(author -> staffServer
                        .when(request("/v3/persons")
                                .withMethod(HttpMethod.GET.name())
                                .withQueryStringParameter("login", author)
                                .withQueryStringParameter("_one", "1")
                        )
                        .respond(response().withStatusCode(HttpStatusCode.OK_200.code())
                                .withBody(JsonBody.json("""
                                        {
                                            "login": "%s",
                                            "official": {
                                                "affiliation": "yandex",
                                                "is_robot": %s
                                            }
                                        }
                                        """.formatted(author, isRobot)
                                )))
                );
    }

    private void verifyStorageRegisterCheckRequest() {
        assertThat(storageApiServiceImpl.checks).isEqualTo(Set.of(
                parseJsonFromFile(
                        "autocheck/postcommits/storage.registerCheckRequest.json",
                        StorageApi.RegisterCheckRequest[].class
                )
        ));
    }

    private void verifyStorageRegisterIterationRequest() {
        assertThat(storageApiServiceImpl.iterations).isEqualTo(Set.of(
                parseJsonFromFile(
                        "autocheck/postcommits/storage.registerIterationRequest.json",
                        StorageApi.RegisterCheckIterationRequest[].class
                )
        ));
    }

    private void verifyStorageRegisterTaskRequest() {
        assertThat(storageApiServiceImpl.tasks).isEqualTo(Set.of(
                parseJsonFromFile(
                        "autocheck/postcommits/storage.registerTasksRequest.json",
                        StorageApi.RegisterTaskRequest[].class
                )
        ));
    }

    private void verifyNoChecksRegistered() {
        assertThat(storageApiServiceImpl.checks).isEmpty();
        assertThat(storageApiServiceImpl.iterations).isEmpty();
        assertThat(storageApiServiceImpl.tasks).isEmpty();
    }

    private void verifyProducedResourcesData(TestJobContext jobContext) {
        var producedResourcesData = new JsonArray();
        jobContext.resources().getProducedResources().stream()
                .map(Resource::getData)
                .forEach(producedResourcesData::add);

        log.info("data: {}", producedResourcesData);
        assertThat(producedResourcesData).isEqualTo(
                parseJsonFromFile("autocheck/postcommits/producedResourcesData.1.json", JsonArray.class, prettyGson)
        );
    }

    private <T> T parseJsonFromFile(String filePath, Class<T> clazz) {
        return parseJsonFromFile(filePath, clazz, this.gson);
    }

    private <T> T parseJsonFromFile(String filePath, Class<T> clazz, Gson gsonInstance) {
        return parseJson(ResourceUtils.textResource(filePath), clazz, gsonInstance);
    }

    private <T> T parseJson(String json, Class<T> clazz, Gson gsonInstance) {
        if (clazz.isArray()) {
            registerFormatterForType(clazz.getComponentType(), gsonInstance::toJson);
        } else {
            registerFormatterForType(clazz, gsonInstance::toJson);
        }
        return gsonInstance.fromJson(json, clazz);
    }

    private static TestJobContext createJobContext(CiProcessId processId, JsonObject flowVars) {
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
                                .previousRevision(TestData.TRUNK_COMMIT_2.toOrderedTrunkArcRevision())
                                .revision(TestData.TRUNK_COMMIT_3.toOrderedTrunkArcRevision())
                                .commit(TestData.TRUNK_COMMIT_3)
                                .build()
                )
                .jobs(Map.of())
                .build();

        return new TestJobContext(flowLaunchEntity);
    }


    @Configuration
    public static class Config {

        @Bean
        ArcService arcService() {
            return new ArcServiceStub(
                    "autocheck/postcommits/test-repo",
                    TestData.TRUNK_COMMIT_2,
                    TestData.TRUNK_COMMIT_3
            );
        }

        @Bean
        StorageApiClient storageApiClient() {
            return StorageApiClient.create(GrpcClientPropertiesStub.of(STORAGE_IN_PROCESS_SERVER));
        }

        @Bean
        ClientAndServer staffServer() {
            return new ClientAndServer();
        }

        @Bean
        public StaffClient staffClient(ClientAndServer staffServer) {
            return StaffClient.create(HttpClientPropertiesStub.of(staffServer));
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

        Set<StorageApi.RegisterCheckRequest> checks = new LinkedHashSet<>();
        Set<StorageApi.RegisterCheckIterationRequest> iterations = new LinkedHashSet<>();
        Set<StorageApi.RegisterTaskRequest> tasks = new LinkedHashSet<>();

        public void reset() {
            checks.clear();
            iterations.clear();
            tasks.clear();
        }

        @Override
        public void registerCheck(StorageApi.RegisterCheckRequest request,
                                  StreamObserver<StorageApi.RegisterCheckResponse> responseObserver) {
            checks.add(request);
            responseObserver.onNext(StorageApi.RegisterCheckResponse.newBuilder()
                    .setCheck(CheckOuterClass.Check.newBuilder()
                            .setId(STORAGE_CHECK_ID)
                            .setDiffSetId(request.getDiffSetId())
                            .setOwner(request.getOwner())
                            .addAllTags(request.getTagsList())
                            .setInfo(request.getInfo())
                            .setLeftRevision(request.getLeftRevision())
                            .setRightRevision(request.getRightRevision())
                            .setDistbuildPriority(request.getDistbuildPriority())
                            .build()
                    )
                    .build());
            responseObserver.onCompleted();
        }

        @Override
        public void registerCheckIteration(StorageApi.RegisterCheckIterationRequest request,
                                           StreamObserver<CheckIteration.Iteration> responseObserver) {
            iterations.add(request);
            responseObserver.onNext(CheckIteration.Iteration.newBuilder()
                    .setId(CheckIteration.IterationId.newBuilder()
                            .setCheckId(request.getCheckId())
                            .setCheckType(request.getCheckType())
                            .setNumber(request.getNumber())
                            .build()
                    )
                    .build());
            responseObserver.onCompleted();
        }

        @Override
        public void registerTask(StorageApi.RegisterTaskRequest request,
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
