package ru.yandex.ci.engine.pcm;

import java.time.Clock;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import NDistBuild.PoolApi;
import NDistBuild.TPoolStorageGrpc;
import io.grpc.inprocess.InProcessServerBuilder;
import io.grpc.stub.StreamObserver;
import lombok.RequiredArgsConstructor;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;
import org.springframework.boot.test.mock.mockito.MockBean;

import ru.yandex.ci.common.grpc.GrpcClientPropertiesStub;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.db.autocheck.model.AccessControl;
import ru.yandex.ci.core.db.autocheck.model.PoolNameByACEntity;
import ru.yandex.ci.core.db.autocheck.model.PoolNodeEntity;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;

class DistBuildPCMServiceTest extends YdbCiTestBase {
    private static final String MOCK_USER = "anyUser";
    private static final AccessControl AC_ID = AccessControl.ofUser(MOCK_USER);

    @MockBean
    AbcService abcService;

    @Test
    void testInsertNewData() throws Exception {
        var pcmService = configurePCMService(List.of(
                List.of("autocheck/test_1", "autocheck/test_2")
        ));

        Set<PoolNode> expected = Set.of(
                new PoolNode(AC_ID, "autocheck/test_1", 0, 6.0),
                new PoolNode(AC_ID, "autocheck/test_2", 0, 15.0)
        );

        pcmService.updatePools();

        Set<PoolNode> actual = dumpAllPoolsFromDb();

        Assertions.assertEquals(expected, actual);
        db.currentOrReadOnly(() -> Assertions.assertEquals(1, db.distBuildPoolNameByAC().countAll()));
    }

    @Test
    void testUnchangedPools() throws Exception {
        var pcmService = configurePCMService(List.of(
                List.of("autocheck/test_1", "autocheck/test_2"),
                List.of("autocheck/test_1", "autocheck/test_2")
        ));

        pcmService.updatePools();

        List<PoolNameByACEntity> insertedByAC = new ArrayList<>();
        Set<PoolNodeEntity> inserted = new HashSet<>();
        db.currentOrReadOnly(() -> {
            insertedByAC.addAll(db.distBuildPoolNameByAC().findAll());
            inserted.addAll(db.distBuildPoolNodes().findAll());
        });

        Assertions.assertEquals(1, insertedByAC.size());
        Assertions.assertEquals(2, insertedByAC.get(0).getPoolNames().size());
        Assertions.assertEquals(2, inserted.size());

        pcmService.updatePools();

        db.currentOrReadOnly(() -> {
            Assertions.assertEquals(Set.copyOf(insertedByAC), Set.copyOf(db.distBuildPoolNameByAC().findAll()));
            Assertions.assertEquals(inserted, Set.copyOf(db.distBuildPoolNodes().findAll()));
        });
    }

    @Test
    void testPoolsChange() throws Exception {
        var pcmService = configurePCMService(List.of(
                List.of("autocheck/test_1"),
                List.of("autocheck/test_2", "autocheck/test_1")
        ));

        var expected = Set.of(
                new PoolNode(AC_ID, "autocheck/test_1", 0, 6.0),
                new PoolNode(AC_ID, "autocheck/test_2", 0, 15.0)
        );

        pcmService.updatePools();

        db.currentOrReadOnly(() -> Assertions.assertEquals(1, db.distBuildPoolNameByAC().countAll()));

        pcmService.updatePools();

        Set<PoolNode> actual = dumpAllPoolsFromDb();

        Assertions.assertEquals(expected, actual);
    }

    @Test
    void testFetchAccumulatedNodes() throws Exception {
        var pcmService = configurePCMService(List.of(List.of("autocheck/test_1")));

        var expected = Set.of(
                new PoolNode(AC_ID, "autocheck/test_1", 0, 6.0)
        );
        var actual = pcmService.fetchPoolNodes().values().stream()
                .flatMap(Collection::stream).collect(Collectors.toSet());

        Assertions.assertEquals(expected, actual);
    }

    @Test
    void testLoadPools() throws Exception {
        Instant oldUpdated = Instant.ofEpochSecond(1614486000L);
        Instant newUpdated = Instant.ofEpochSecond(1646022000L);
        var outdatedData = List.of(
                new PoolNode(AC_ID, "autocheck/old_1", 0, 1.0),
                new PoolNode(AC_ID, "autocheck/old_2", 0, 25500.0)
        );
        var newData = List.of(
                new PoolNode(AC_ID, "autocheck/new_1", 0, 25500.0),
                new PoolNode(AC_ID, "autocheck/new_2", 0, 1.0)
        );

        insertPoolNodesInDb(outdatedData, oldUpdated);
        insertPoolNodesInDb(newData, newUpdated);

        var pcmService = configurePCMService(List.of());

        Assertions.assertEquals(newData, pcmService.getAvailablePools(List.of(AC_ID)));
    }

    private DistBuildPCMService configurePCMService(List<List<String>> treeNamesListResponses) throws Exception {
        var serverName = InProcessServerBuilder.generateName();
        InProcessServerBuilder.forName(serverName)
                .directExecutor()
                .addService(new PCMServiceGrpc(treeNamesListResponses))
                .build()
                .start();

        var properties = GrpcClientPropertiesStub.of(serverName);
        return new DistBuildPCMService(
                Clock.systemUTC(),
                properties,
                abcService,
                db,
                null,
                true
        );
    }

    private Set<PoolNode> dumpAllPoolsFromDb() {
        Set<PoolNode> actual = new HashSet<>();
        db.currentOrReadOnly(() -> actual.addAll(
                db.distBuildPoolNameByAC().findAll().stream()
                        .flatMap(
                                e -> db.distBuildPoolNodes().findAll().stream()
                                        .filter(n -> e.getPoolNames().contains(n.getId().getName()))
                                        .map(n -> new PoolNode(e.getId().getAc(), n))
                        ).collect(Collectors.toSet())
        ));

        return actual;
    }

    private void insertPoolNodesInDb(List<PoolNode> pools, Instant updated) {
        List<AccessControl> acIds = pools.stream()
                .map(PoolNode::getAcId)
                .distinct()
                .collect(Collectors.toList());
        List<PoolNameByACEntity> poolNamesByAC = acIds.stream()
                .map(ac -> PoolNameByACEntity.builder()
                        .id(new PoolNameByACEntity.Id(updated, ac))
                        .poolNames(pools.stream()
                                .filter(p -> ac.equals(p.getAcId()))
                                .map(PoolNode::getPoolPath)
                                .collect(Collectors.toList()))
                        .build())
                .collect(Collectors.toList());

        List<PoolNodeEntity> poolNodeEntities = pools.stream()
                .map(p -> p.toPoolNodeEntity(updated))
                .collect(Collectors.toList());

        db.currentOrTx(() -> {
            db.distBuildPoolNameByAC().save(poolNamesByAC);
            db.distBuildPoolNodes().save(poolNodeEntities);
        });
    }

    @RequiredArgsConstructor
    private static class PCMServiceGrpc extends TPoolStorageGrpc.TPoolStorageImplBase {
        private final List<List<String>> poolNames;
        private int responseCounter = 0;

        private final PoolApi.TACL acl = PoolApi.TACL.newBuilder()
                .addACItem(
                        PoolApi.TACItem.newBuilder()
                                .setUser(PoolApi.TACUser.newBuilder().setName(MOCK_USER).build())
                                .setPermissions(PoolApi.TACPermissions.newBuilder().setUse(true).build())
                                .build()
                )
                .build();

        private final Map<String, PoolApi.TLocationConfigs> pools = Map.of(
                "autocheck/test_1",
                PoolApi.TLocationConfigs.newBuilder()
                        .putLocationConfigs("//sas_gg", PoolApi.TBriefPoolInfo.newBuilder()
                                .setACL(acl)
                                .setWeight(1.0)
                                .build()
                        )
                        .putLocationConfigs("//sas", PoolApi.TBriefPoolInfo.newBuilder()
                                .setACL(acl)
                                .setWeight(2.0)
                                .build()
                        )
                        .putLocationConfigs("//vla", PoolApi.TBriefPoolInfo.newBuilder()
                                .setACL(acl)
                                .setWeight(3.0)
                                .build()
                        )
                        .build(),
                "autocheck/test_2",
                PoolApi.TLocationConfigs.newBuilder()
                        .putLocationConfigs("//sas_gg", PoolApi.TBriefPoolInfo.newBuilder()
                                .setACL(acl)
                                .setWeight(4.0)
                                .build()
                        )
                        .putLocationConfigs("//sas", PoolApi.TBriefPoolInfo.newBuilder()
                                .setACL(acl)
                                .setWeight(5.0)
                                .build()
                        )
                        .putLocationConfigs("//vla", PoolApi.TBriefPoolInfo.newBuilder()
                                .setACL(acl)
                                .setWeight(6.0)
                                .build()
                        )
                        .build()
        );

        @Override
        public void getRunnablePools(PoolApi.TRunnablePoolsRequest request,
                                     StreamObserver<PoolApi.TRunnablePoolsResponse> responseObserver) {
            var response = PoolApi.TRunnablePoolsResponse.newBuilder();
            for (var poolName : poolNames.get(responseCounter++)) {
                response.putPools(poolName, pools.get(poolName));
            }

            responseObserver.onNext(response.build());
            responseObserver.onCompleted();
        }
    }
}
