//TODO move usefull part to AutocheckServiceTest
//package ru.yandex.ci.engine.autocheck;
//
//import java.io.IOException;
//import java.nio.file.Path;
//
//import io.grpc.inprocess.InProcessChannelBuilder;
//import io.grpc.inprocess.InProcessServerBuilder;
//import io.grpc.stub.StreamObserver;
//import org.junit.jupiter.api.BeforeAll;
//import org.junit.jupiter.api.Test;
//import org.mockito.Mockito;
//
//import ru.yandex.ci.core.arc.ArcRevision;
//import ru.yandex.ci.core.arc.OrderedArcRevision;
//import ru.yandex.ci.core.config.FlowFullId;
//import ru.yandex.ci.flow.engine.runtime.state.model.LaunchInfo;
//import ru.yandex.ci.core.launch.LaunchFlowInfo;
//import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
//import ru.yandex.ci.core.launch.LaunchVcsInfo;
//import ru.yandex.ci.core.security.YavToken;
//import ru.yandex.ci.engine.autocheck.storage.StorageApiClient;
//import ru.yandex.ci.flow.db.CiDb;
//import ru.yandex.ci.flow.engine.definition.context.impl.LaunchJobContext;
//import ru.yandex.ci.flow.engine.definition.context.impl.UpstreamResourcesCollector;
//import ru.yandex.ci.flow.engine.runtime.di.LaunchEntitiesFactory;
//import ru.yandex.ci.flow.engine.runtime.di.ResourceService;
//import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
//import ru.yandex.ci.flow.engine.runtime.state.JobProgressService;
//import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
//import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
//import ru.yandex.ci.flow.engine.source_code.SourceCodeService;
//import ru.yandex.ci.storage.api.StorageApi;
//import ru.yandex.ci.storage.api.StorageApiServiceGrpc;
//import ru.yandex.ci.storage.core.CheckIteration;
//import ru.yandex.ci.storage.core.CheckOuterClass;
//import ru.yandex.ci.storage.core.CheckTaskOuterClass;
//
//import static org.assertj.core.api.Assertions.assertThat;
//import static org.mockito.AdditionalAnswers.delegatesTo;
//
//public class CiStorageRegisterJobTest {
//    private static final StorageApiServiceGrpc.StorageApiServiceImplBase SERVICE_IMPL = Mockito.mock(
//            StorageApiServiceGrpc.StorageApiServiceImplBase.class,
//            delegatesTo(new StorageApiServiceGrpc.StorageApiServiceImplBase() {
//                @Override
//                public void registerCheck(StorageApi.RegisterCheckRequest request,
//                                          StreamObserver<StorageApi.RegisterCheckResponse> responseObserver) {
//                    responseObserver.onNext(StorageApi.RegisterCheckResponse.newBuilder()
//                            .setCheck(CheckOuterClass.Check.newBuilder()
//                                    .setOwner("owner")
//                                    .build()
//                            ).build()
//                    );
//                    responseObserver.onCompleted();
//                }
//
//                @Override
//                public void registerCheckIteration(StorageApi.RegisterCheckIterationRequest request,
//                                                   StreamObserver<CheckIteration.Iteration> responseObserver) {
//                    responseObserver.onNext(CheckIteration.Iteration.newBuilder().build());
//                    responseObserver.onCompleted();
//                }
//
//                @Override
//                public void registerTask(StorageApi.RegisterTaskRequest request,
//                                         StreamObserver<CheckTaskOuterClass.CheckTask> responseObserver) {
//                    responseObserver.onNext(CheckTaskOuterClass.CheckTask.newBuilder().build());
//                    responseObserver.onCompleted();
//                }
//            }));
//
//    private static CiStorageRegisterJob ciStorageRegisterJob;
//
//    @BeforeAll
//    static void setup() throws IOException {
//        var serverName = InProcessServerBuilder.generateName();
//        InProcessServerBuilder.forName(serverName).directExecutor().addService(SERVICE_IMPL).build().start();
//        var channel = InProcessChannelBuilder.forName(serverName).directExecutor().build();
//        var storageApiClient = new StorageApiClient(channel);
//        ciStorageRegisterJob = new CiStorageRegisterJob(storageApiClient);
//    }
//
//    @Test
//    void getSourceCodeId() {
//        assertThat(ciStorageRegisterJob.getSourceCodeId().toString()).isNotEmpty();
//    }
//
//    @Test
//    void executeJob() throws Exception {
//        var launchFlowInfo = new LaunchFlowInfo(
//                OrderedArcRevision.fromRevision(ArcRevision.of("asdf"), "trunk", 1, 1),
//                FlowFullId.of(Path.of("trunk/ci"), "myid"), "stage group id",
//                new LaunchRuntimeInfo(YavToken.Id.of("test"), "test"));
//
//        var launchVcsInfo = new LaunchVcsInfo(
//                OrderedArcRevision.fromRevision(ArcRevision.of("asdf"), "trunk", 1, 1),
//                null, OrderedArcRevision.fromRevision(ArcRevision.of("asdf"), "trunk", 1, 1),
//                null, 0, null, null);
//
//        var flowLaunchEntity = FlowLaunchEntity.builder()
//                .id("f id")
//                .processId("p id")
//                .projectId("projectid")
//                .flowInfo(launchFlowInfo)
//                .vcsInfo(launchVcsInfo)
//                .launchInfo(new LaunchInfo("1"))
//                .build();
//
//        var db = Mockito.mock(CiDb.class);
//        var upstreamResourcesCollector = Mockito.mock(UpstreamResourcesCollector.class);
//        var jobProgressService = Mockito.mock(JobProgressService.class);
//        var jobState = Mockito.mock(JobState.class);
//        Mockito.when(jobState.getJobId()).thenReturn("BUILD_TRUNK_META_LINUX_DISTBUILD");
//        var flowStateService = Mockito.mock(FlowStateService.class);
//        var launchEntitiesFactory = Mockito.mock(LaunchEntitiesFactory.class);
//        var resourceService = Mockito.mock(ResourceService.class);
//        var sourceCodeService = Mockito.mock(SourceCodeService.class);
//
//        var jobContext = LaunchJobContext.builder()
//                .withDb(db)
//                .withUpstreamResourcesCollector(upstreamResourcesCollector)
//                .withJobProgressService(jobProgressService)
//                .withFlowLaunch(flowLaunchEntity)
//                .withJobState(jobState)
//                .withFlowStateService(flowStateService)
//                .withLaunchEntitiesFactory(launchEntitiesFactory)
//                .withResourceService(resourceService)
//                .withSourceCodeEntityService(sourceCodeService)
//                .withLaunchNumber(1)
//                .build();
//        ciStorageRegisterJob.execute(jobContext);
//        var firstResource = jobContext.resources().getProducedResources().stream().findFirst();
//        assertThat(firstResource).isPresent();
//        var resource = firstResource.get();
//        assertThat(resource.getResourceType().getMessageName()).isEqualTo("ci.common.CiStorageInfo");
//    }
//}
