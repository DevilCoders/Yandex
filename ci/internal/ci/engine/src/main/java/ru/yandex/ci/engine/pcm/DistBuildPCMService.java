package ru.yandex.ci.engine.pcm;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Function;
import java.util.function.Supplier;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import NDistBuild.PoolApi;
import NDistBuild.TPoolStorageGrpc;
import NDistBuild.TPoolStorageGrpc.TPoolStorageBlockingStub;
import com.google.common.annotations.VisibleForTesting;
import com.google.common.collect.Sets;
import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.grpc.GrpcClient;
import ru.yandex.ci.common.grpc.GrpcClientImpl;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.abc.ServiceSlugWithRoles;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.autocheck.model.AccessControl;
import ru.yandex.ci.core.db.autocheck.model.PoolNameByACEntity;
import ru.yandex.ci.core.db.autocheck.model.PoolNodeEntity;
import ru.yandex.lang.NonNullApi;

/**
 * Сервис для определения автосборочной квоты доступной пользователю
 */
@Slf4j
@NonNullApi
public class DistBuildPCMService implements PCMService, AutoCloseable {
    private final Clock clock;
    private final GrpcClient grpcClient;
    private final Supplier<TPoolStorageBlockingStub> poolManager;
    private final AbcService abcService;
    private final CiMainDb db;

    public DistBuildPCMService(
            Clock clock,
            GrpcClientProperties grpcClientProperties,
            AbcService abcService,
            CiMainDb db,
            @Nullable MeterRegistry meterRegistry,
            boolean bootstrap
    ) {
        this.clock = clock;
        this.grpcClient = GrpcClientImpl.builder(grpcClientProperties, getClass()).build();
        this.poolManager = grpcClient.buildStub(TPoolStorageGrpc::newBlockingStub);
        this.abcService = abcService;
        this.db = db;
        if (bootstrap) {
            this.loadPools();

            if (meterRegistry != null) {
                Gauge.builder(
                        "dist_build_pcm_service_update_lag",
                        () -> Duration.between(this.getLastUpdatedTime(), clock.instant()).toSeconds()
                ).tag("time_unit", "seconds").register(meterRegistry);
            }
        }
    }

    @Override
    public void close() throws Exception {
        grpcClient.close();
    }

    @Override
    public void updatePools() {
        Map<AccessControl, List<PoolNode>> byAccessControls;
        try {
            byAccessControls = fetchPoolNodes();
        } catch (Exception ex) {
            log.error("Unable fetch pools from PCM GRPC");
            throw ex;
        }

        if (isPoolsChanged(byAccessControls)) {
            log.info("Pools changed, new pools by groups: {}", byAccessControls);
            Instant updated = clock.instant();
            db.currentOrTx(() -> {
                log.info("Updating pools in YDB...");
                db.distBuildPoolNameByAC().save(
                        byAccessControls.entrySet().stream()
                                .map(e -> PoolNameByACEntity.builder()
                                        .id(new PoolNameByACEntity.Id(updated, e.getKey()))
                                        .poolNames(e.getValue().stream()
                                                .map(PoolNode::getPoolPath).collect(Collectors.toList())
                                        ).build()
                                ).collect(Collectors.toList())
                );

                db.distBuildPoolNodes().save(
                        byAccessControls.values().stream()
                                .flatMap(Collection::stream)
                                .map(n -> n.toPoolNodeEntity(updated))
                                .distinct()
                                .collect(Collectors.toList())
                );
            });

            log.info("Pools updated successfully with updated Tm {}", updated);
        }
    }

    @Override
    public List<PoolNode> getAvailablePools(List<AccessControl> acIds) {
        Map<String, PoolNodeEntity> poolNodeEntities = new HashMap<>();
        List<PoolNameByACEntity> poolNames = new ArrayList<>();

        db.currentOrReadOnly(() -> db.distBuildPoolNameByAC().findLastUpdateTime().ifPresent(tm -> {
            poolNames.addAll(
                    db.distBuildPoolNameByAC().find(
                            acIds.stream().map(acId -> new PoolNameByACEntity.Id(tm, acId)).collect(Collectors.toSet())
                    )
            );

            poolNodeEntities.putAll(
                    db.distBuildPoolNodes().find(poolNames.stream()
                            .flatMap(byAc -> byAc.getPoolNames().stream())
                            .distinct()
                            .map(pn -> new PoolNodeEntity.Id(tm, pn))
                            .collect(Collectors.toSet())
                    ).stream().collect(Collectors.toMap(e -> e.getId().getName(), e -> e))
            );
        }));

        return poolNames.stream().flatMap(
                e -> e.getPoolNames().stream()
                        .map(p -> new PoolNode(e.getId().getAc(), poolNodeEntities.get(p)))
        ).collect(Collectors.toList());
    }

    public Instant getLastUpdatedTime() {
        Instant[] updated = {null};
        db.currentOrReadOnly(
                () -> updated[0] = db.distBuildPoolNameByAC().findLastUpdateTime().orElse(Instant.ofEpochSecond(0))
        );

        return updated[0];
    }

    @VisibleForTesting
    Map<AccessControl, List<PoolNode>> fetchPoolNodes() {
        List<PoolNode> nodes = poolManager.get()
                .getRunnablePools(PoolApi.TRunnablePoolsRequest.newBuilder().setPoolPrefix("autocheck").build())
                .getPoolsMap().entrySet().stream()
                .flatMap(e -> getAccumulatedPoolNodes(e.getKey(), e.getValue()))
                .collect(Collectors.toList());

        return byAccessControlOrdered(nodes);
    }

    private Map<AccessControl, List<PoolNode>> loadPools() {
        var lastUpdateTime = db.currentOrReadOnly(() -> db.distBuildPoolNameByAC().findLastUpdateTime());
        if (lastUpdateTime.isPresent()) {
            return db.scan().run(() -> {
                var tm = lastUpdateTime.get();
                var poolNodesByName = db.distBuildPoolNodes().listPoolsByUpdateTime(tm).stream()
                        .collect(Collectors.toMap(n -> n.getId().getName(), Function.identity()));
                return db.distBuildPoolNameByAC().listPoolsByUpdateTime(tm).stream()
                        .collect(Collectors.toMap(
                                p -> p.getId().getAc(),
                                p -> p.getPoolNames().stream()
                                        .map(n -> new PoolNode(p.getId().getAc(), poolNodesByName.get(n)))
                                        .collect(Collectors.toList())
                        ));
            });
        } else {
            return Map.of();
        }
    }

    /**
     * Groups by access controls and order according {@code comparePoolNodes(...)}
     *
     * @param nodes pool nodes
     * @return grouped and sorted pool nodes
     */
    private Map<AccessControl, List<PoolNode>> byAccessControlOrdered(List<PoolNode> nodes) {
        Map<AccessControl, List<PoolNode>> byAC = new HashMap<>();

        for (var node : nodes) {
            byAC.computeIfAbsent(node.getAcId(), k -> new ArrayList<>()).add(node);
        }

        byAC.values().forEach(l -> l.sort(PoolNode::compare));

        return byAC;
    }

    private Stream<PoolNode> getAccumulatedPoolNodes(String poolName, PoolApi.TLocationConfigs configs) {
        Set<AccessControl> acs = null;
        int[] slots = {0};
        double[] weight = {0};

        for (var config : configs.getLocationConfigsMap().values()) {
            if (acs == null) {
                acs = config.getACL().getACItemList().stream()
                        .flatMap(this::toAccessControl).collect(Collectors.toSet());
            }

            acs = Sets.intersection(
                    acs,
                    config.getACL().getACItemList().stream().flatMap(this::toAccessControl).collect(Collectors.toSet())
            );

            slots[0] += config.getResourceGuarantees().getSlotsCount();
            weight[0] += config.getWeight();
        }

        if (acs != null) {
            return acs.stream().map(ac -> new PoolNode(ac, poolName, slots[0], weight[0]));
        }

        return Stream.of();
    }

    private Stream<AccessControl> toAccessControl(PoolApi.TACItem item) {
        //Skip AccessControl items without use permissions
        if (!item.getPermissions().getUse()) {
            return Stream.of();
        }

        Stream.Builder<AccessControl> aclBuilder = Stream.builder();
        if (item.hasAllStaff()) {
            aclBuilder.accept(AccessControl.ALL_STAFF);
        }
        if (item.hasAbc()) {
            //Means any role
            if (item.getAbc().getRoleCount() == 0) {
                aclBuilder.accept(AccessControl.ofAbc(item.getAbc().getName(), ""));
            }

            for (var role : item.getAbc().getRoleList()) {
                aclBuilder.accept(AccessControl.ofAbc(item.getAbc().getName(), role));
            }

            //Recursive means that we need to include also all subservices
            if (item.getAbc().getRecursive()) {
                abcService.getServiceMembersWithDescendants(
                        new ServiceSlugWithRoles(item.getAbc().getName(), item.getAbc().getRoleList())
                ).forEach(
                        acl -> aclBuilder.accept(AccessControl.ofAbc(
                                acl.getServiceSlug(),
                                item.getAbc().getRoleCount() > 0 ? acl.getRoleCode() : ""
                        ))
                );
            }
        }
        if (item.hasUser()) {
            aclBuilder.accept(AccessControl.ofUser(item.getUser().getName()));
        }

        return aclBuilder.build();
    }

    private boolean isPoolsChanged(Map<AccessControl, List<PoolNode>> newPoolsByAC) {
        var poolsByAccessControl = loadPools();
        if (poolsByAccessControl.size() != newPoolsByAC.size()
                || !Sets.difference(poolsByAccessControl.keySet(), newPoolsByAC.keySet()).isEmpty()) {
            return true;
        }

        for (var entry : poolsByAccessControl.entrySet()) {
            if (!entry.getValue().equals(newPoolsByAC.get(entry.getKey()))) {
                return true;
            }
        }

        return false;
    }
}
