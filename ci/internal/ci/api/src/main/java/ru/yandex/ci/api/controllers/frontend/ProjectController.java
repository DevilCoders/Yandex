package ru.yandex.ci.api.controllers.frontend;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.function.Function;
import java.util.function.Supplier;

import javax.annotation.Nullable;

import com.google.common.base.Suppliers;
import io.grpc.stub.StreamObserver;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;

import ru.yandex.ci.api.internal.frontend.project.FrontendProjectApi;
import ru.yandex.ci.api.internal.frontend.project.ProjectServiceGrpc;
import ru.yandex.ci.api.misc.AuthUtils;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.abc.AbcServiceEntity;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.BranchInfo;
import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.config.ConfigProblem;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.db.model.FavoriteProject;
import ru.yandex.ci.core.db.model.VirtualConfigState;
import ru.yandex.ci.core.db.table.ProjectFilter;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.launch.auto.AutoReleaseService;
import ru.yandex.ci.engine.notification.xiva.ProjectStatisticsChangedEvent;
import ru.yandex.ci.engine.notification.xiva.XivaNotifier;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.util.OffsetResults;

@Slf4j
public class ProjectController extends ProjectServiceGrpc.ProjectServiceImplBase {

    public static final int TOP_N_BRANCHES_IN_PROJECT = 3;

    private final AbcService abcService;
    private final ArcService arcService;
    private final CiMainDb db;
    private final ConfigStateBuilder configStateBuilder;
    private final XivaNotifier xivaNotifier;

    public ProjectController(
            AbcService abcService,
            ArcService arcService,
            CiMainDb db,
            AutoReleaseService autoReleaseService,
            BranchService branchService,
            XivaNotifier xivaNotifier
    ) {
        this.abcService = abcService;
        this.arcService = arcService;
        this.db = db;
        this.configStateBuilder = new ConfigStateBuilder(db, autoReleaseService, branchService);
        this.xivaNotifier = xivaNotifier;
    }

    @Override
    public void getProjects(FrontendProjectApi.GetProjectsRequest request,
                            StreamObserver<FrontendProjectApi.GetProjectsResponse> responseObserver) {

        var projectFilter = getProjectFilter(request);
        var results = getProjects(request, projectFilter);

        List<FrontendProjectApi.ListProject> projects = toProtoListProjects(
                results.items(),
                request.getOnlyFavorite(),
                getFavoriteProjects(request)
        );

        responseObserver.onNext(
                FrontendProjectApi.GetProjectsResponse.newBuilder()
                        .addAllProjects(projects)
                        .setOffset(ProtoMappers.toProtoOffset(results))
                        .build()
        );

        responseObserver.onCompleted();
    }

    @Override
    public void getConfigStates(
            FrontendProjectApi.GetConfigStatesRequest request,
            StreamObserver<FrontendProjectApi.GetConfigStatesResponse> responseObserver
    ) {

        var configStates = db.scan().run(() -> db.configStates().findAll(request.getIncludeInvalidConfigs()))
                .stream()
                .sorted(Comparator.comparing(ConfigState::getConfigPath))
                .toList();

        var projectIds = configStates.stream()
                .map(ConfigState::getProject)
                .filter(Objects::nonNull)
                .sorted()
                .distinct()
                .toList();

        var services = abcService.getServices(projectIds);
        var protoConfigStates = configStateBuilder.toProtoConfigStateByProject(configStates, false);
        var result = FrontendProjectApi.GetConfigStatesResponse.newBuilder();
        for (var projectId : projectIds) {
            List<Common.ConfigState> projectConfigStates = protoConfigStates.get(projectId);
            var serviceInfo = services.get(projectId);
            if (serviceInfo == null) {
                continue;
            }
            result.addProjects(FrontendProjectApi.ProjectConfigs.newBuilder()
                    .setProject(ProtoMappers.toProtoProject(serviceInfo))
                    .addAllConfigs(projectConfigStates)
                    .build());
        }
        var response = result.build();

        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void getProject(
            FrontendProjectApi.GetProjectRequest request,
            StreamObserver<FrontendProjectApi.GetProjectResponse> responseObserver
    ) {
        String projectId = request.getProjectId();

        var serviceInfo = abcService.getService(projectId);
        if (serviceInfo.isEmpty()) {
            throw GrpcUtils.notFoundException("No abc service with id: " + projectId);
        }

        var response = FrontendProjectApi.GetProjectResponse.newBuilder();
        response.setProject(ProtoMappers.toProtoProject(serviceInfo.get()));

        var configStates = collectConfigs(request);

        if (configStates.isEmpty()) {
            // It's OK for the first project in PR, no need to raise an exception here
            log.warn("No configs associated with abc service, skip processing configs: {}", projectId);
        } else {

            var filteredConfigs = filterConfigs(configStates, request.getIncludeInvalidConfigs());

            var expectReleases = getProcessTypes(request).contains(Common.VirtualProcessType.VP_NONE);
            var protoConfigState = configStateBuilder.toProtoConfigStateByProject(filteredConfigs, expectReleases)
                    .values()
                    .stream()
                    .flatMap(Collection::stream)
                    .toList();
            response.addAllConfigs(protoConfigState);
        }

        response.setXivaSubscription(
                xivaNotifier.toXivaSubscription(new ProjectStatisticsChangedEvent(projectId))
        );

        responseObserver.onNext(response.build());
        responseObserver.onCompleted();
    }


    @Override
    public void getProjectInfo(
            FrontendProjectApi.GetProjectInfoRequest request,
            StreamObserver<FrontendProjectApi.GetProjectInfoResponse> responseObserver
    ) {

        var responseBuilder = FrontendProjectApi.GetProjectInfoResponse.newBuilder();
        for (var stat : collectStats(request).entrySet()) {
            responseBuilder.addProcessTypeDetailsBuilder()
                    .setProcessType(stat.getKey())
                    .setConfigCount(stat.getValue().intValue());
        }

        responseObserver.onNext(responseBuilder.build());
        responseObserver.onCompleted();
    }

    @Override
    public void getConfigHistory(FrontendProjectApi.GetConfigHistoryRequest request,
                                 StreamObserver<FrontendProjectApi.GetConfigHistoryResponse> responseObserver) {

        ArcBranch targetBranch = request.hasBranch()
                ? ArcBranch.ofString(request.getBranch().getValue())
                : ArcBranch.trunk();
        if (targetBranch.isUnknown()) {
            throw GrpcUtils.invalidArgumentException(
                    "Unknown type of branch: " + request.getBranch().getValue()
            );
        }
        Path configPath = AYamlService.dirToConfigPath(request.getConfigDir());

        var response = db.currentOrReadOnly(() -> {
            OffsetResults<ConfigEntity> results;
            if (targetBranch.isPr()) {
                results = getConfigHistoryForPrBranch(
                        targetBranch,
                        configPath,
                        request.getOffsetCommitNumber(),
                        request.hasOffsetCommitHash() ? request.getOffsetCommitHash().getValue() : null,
                        request.getLimit()
                );
            } else {
                Optional<ConfigEntity> branchConfig;
                if (targetBranch.isRelease()) {
                    branchConfig = getBranchConfigEntity(targetBranch, configPath);
                } else {
                    branchConfig = Optional.empty();
                }
                results = OffsetResults.builder()
                        .withItems(
                                request.getLimit(),
                                limit -> {
                                    var configs = db.configHistory().list(
                                            configPath,
                                            targetBranch,
                                            request.getOffsetCommitNumber(),
                                            limit
                                    );
                                    branchConfig.ifPresent(configs::add);
                                    return configs;
                                }
                        )
                        .withTotal(() -> (branchConfig.isPresent() ? 1 : 0) +
                                db.configHistory().count(configPath, targetBranch))
                        .fetch();
            }

            var builder = FrontendProjectApi.GetConfigHistoryResponse.newBuilder()
                    .addAllConfigEntities(
                            StreamEx.of(results.items()).map(this::toProtoConfigEntity).toList()
                    )
                    .setOffset(ProtoMappers.toProtoOffset(results));

            getLastValidConfig(configPath, results.items(), request.getOffsetCommitNumber(), targetBranch)
                    .map(this::toProtoConfigEntity)
                    .ifPresent(builder::setLastValidEntity);

            return builder.build();
        });

        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void addFavoriteProject(FrontendProjectApi.AddFavoriteProjectRequest request,
                                   StreamObserver<FrontendProjectApi.AddFavoriteProjectResponse> observer) {
        updateFavorite(request.getProjectId(), FavoriteProject.Mode.SET);
        observer.onNext(FrontendProjectApi.AddFavoriteProjectResponse.getDefaultInstance());
        observer.onCompleted();
    }

    @Override
    public void removeFavoriteProject(FrontendProjectApi.RemoveFavoriteProjectRequest request,
                                      StreamObserver<FrontendProjectApi.RemoveFavoriteProjectResponse> observer) {
        updateFavorite(request.getProjectId(), FavoriteProject.Mode.UNSET);
        observer.onNext(FrontendProjectApi.RemoveFavoriteProjectResponse.getDefaultInstance());
        observer.onCompleted();
    }

    private void updateFavorite(String project, FavoriteProject.Mode mode) {
        String user = AuthUtils.getUsername();
        FavoriteProject favoriteProject = FavoriteProject.of(user, project, mode);
        log.info("Updating FavoriteProject {}", favoriteProject);
        db.currentOrTx(() -> db.favoriteProjects().save(favoriteProject));
    }

    private OffsetResults<String> getProjects(
            FrontendProjectApi.GetProjectsRequest request,
            @Nullable ProjectFilter projectFilter
    ) {
        if (request.getOnlyFavorite()) {
            return getFavoriteProjectsResults(request, projectFilter);
        } else {
            return getProjectsResults(request, projectFilter);
        }
    }

    @Nullable
    private ProjectFilter getProjectFilter(FrontendProjectApi.GetProjectsRequest request) {
        if (request.getFilter().isEmpty()) {
            return null;
        } else {
            var lookup = "%" + request.getFilter().toLowerCase() + "%";
            var projects = db.currentOrReadOnly(() -> db.abcServices().findServiceSlugs(lookup));
            return ProjectFilter.of(lookup, projects);
        }
    }

    private Set<String> getFavoriteProjects(FrontendProjectApi.GetProjectsRequest request) {
        String user = AuthUtils.getUsername();
        return Set.copyOf(db.currentOrReadOnly(() ->
                db.favoriteProjects().listForUser(user, null, request.getOffsetProjectId(), request.getLimit())));
    }

    private OffsetResults<String> getFavoriteProjectsResults(
            FrontendProjectApi.GetProjectsRequest request,
            @Nullable ProjectFilter projectFilter
    ) {
        var user = AuthUtils.getUsername();
        var offset = request.getOffsetProjectId();
        return OffsetResults.builder()
                .withTransactionReadOnly(db)
                .withItems(
                        request.getLimit(),
                        limit -> db.favoriteProjects().listForUser(user, projectFilter, offset, limit)
                )
                .withTotal(() -> db.favoriteProjects().countForUser(user, projectFilter))
                .fetch();
    }

    private OffsetResults<String> getProjectsResults(
            FrontendProjectApi.GetProjectsRequest request,
            @Nullable ProjectFilter projectFilter
    ) {
        var includeInvalid = request.getIncludeInvalidConfigs();
        var offset = request.getOffsetProjectId();
        return OffsetResults.builder()
                .withTransactionReadOnly(db)
                .withItems(
                        request.getLimit(),
                        limit -> db.configStates().listProjects(projectFilter, offset, includeInvalid, limit)
                )
                .withTotal(() -> db.configStates().countProjects(projectFilter, includeInvalid))
                .fetch();
    }

    private Set<Common.VirtualProcessType> getProcessTypes(FrontendProjectApi.GetProjectRequest request) {
        var configTypes = request.getVirtualProcessTypeCount() > 0
                ? request.getVirtualProcessTypeList()
                : List.of(Common.VirtualProcessType.VP_NONE);
        return new LinkedHashSet<>(configTypes);
    }

    private Map<Common.VirtualProcessType, Long> collectStats(FrontendProjectApi.GetProjectInfoRequest request) {
        return db.currentOrReadOnly(() -> {
            var result = new LinkedHashMap<Common.VirtualProcessType, Long>();
            result.put(Common.VirtualProcessType.VP_NONE,
                    db.configStates().countByProject(
                            request.getProjectId(),
                            request.getIncludeInvalidConfigs()
                    ));
            for (var type : VirtualCiProcessId.VirtualType.values()) {
                result.put(ProtoMappers.toVirtualProcessType(type), 0L);
            }
            for (var e : db.virtualConfigStates().countByProject(request.getProjectId()).entrySet()) {
                result.put(ProtoMappers.toVirtualProcessType(e.getKey()), e.getValue());
            }
            return result;
        });
    }

    private List<ConfigState> collectConfigs(FrontendProjectApi.GetProjectRequest request) {
        // Minor optimization, both configs are usually requested at the same time
        Supplier<List<VirtualConfigState>> virtualStates = Suppliers.memoize(() ->
                db.virtualConfigStates().findByProject(request.getProjectId()));

        Function<VirtualType, List<ConfigState>> getVirtualStates = type ->
                virtualStates.get().stream()
                        .filter(state -> state.getVirtualType() == type)
                        .map(VirtualConfigState::toConfigState)
                        .toList();

        var includeInvalid = request.getIncludeInvalidConfigs();
        var processTypes = getProcessTypes(request);
        return db.currentOrReadOnly(() -> processTypes.stream()
                .map(type -> switch (type) {
                    case VP_NONE -> db.configStates().findByProject(request.getProjectId(), includeInvalid);
                    case VP_LARGE_TESTS -> getVirtualStates.apply(VirtualType.VIRTUAL_LARGE_TEST);
                    case VP_NATIVE_BUILDS -> getVirtualStates.apply(VirtualType.VIRTUAL_NATIVE_BUILD);
                    case UNRECOGNIZED -> throw new IllegalStateException("Unsupported virtual process type: " + type);
                })
                .flatMap(Collection::stream)
                .toList());
    }

    private List<ConfigState> filterConfigs(List<ConfigState> configs, boolean includeInvalid) {
        var normalStates = new ArrayList<ConfigState>(configs.size());
        var draftStates = new ArrayList<ConfigState>();

        for (var config : configs) {
            if (config.getStatus() == ConfigState.Status.DRAFT) {
                draftStates.add(config);
            } else if (includeInvalid || config.getStatus() != ConfigState.Status.NOT_CI) {
                normalStates.add(config);
            }
        }

        // Accept configs if there are only DRAFT states, so UI will work
        // Exclude configs in DRAFT state if any non-DRAFT found
        return normalStates.isEmpty()
                ? draftStates
                : normalStates;
    }

    private Optional<ConfigEntity> getBranchConfigEntity(ArcBranch targetBranch, Path configPath) {
        var config = db.branches().get(BranchInfo.Id.of(targetBranch)).getConfigRevision();
        if (config == null) {
            return Optional.empty();
        }
        return Optional.of(db.configHistory().getById(configPath, config));
    }

    /**
     * @return Если в pr'е конфиг менялся, то в возвращаемом списке конфиг из pr'а + конфиги из апстрима
     * Если в pr'е конфиг не менялся, то возвращаем конфиги из апстрима.
     */
    private OffsetResults<ConfigEntity> getConfigHistoryForPrBranch(
            ArcBranch targetBranch,
            Path configPath,
            long offsetCommitNumber,
            @Nullable String offsetCommitHash,
            int limit
    ) {
        PullRequestDiffSet diffSet = db.pullRequestDiffSetTable()
                .findLatestByPullRequestId(targetBranch.getPullRequestId())
                .orElseThrow(() -> GrpcUtils.invalidArgumentException(
                        "Pull request not found " + targetBranch.getPullRequestId()
                ));

        OrderedArcRevision mergeRevision = diffSet.getOrderedMergeRevision();
        ConfigEntity configAtMergeRevision = db.configHistory().findLastConfig(configPath, mergeRevision)
                .filter(configEntity ->
                        // filter configs from not latest pr iterations
                        configEntity.getRevision().getCommitId().equals(mergeRevision.getCommitId())
                )
                .orElse(null);

        // add configs from trunk/release-branch
        ArcBranch upstreamBranch = diffSet.getVcsInfo().getUpstreamBranch();

        long patchedOffset;
        if (offsetCommitHash != null && offsetCommitHash.equals(mergeRevision.getCommitId())) {
            // patch offset, because we insert configAtMergeRevision in the begin of the result list
            patchedOffset = 0;
        } else {
            patchedOffset = offsetCommitNumber;
        }

        return OffsetResults.builder()
                .withItems(
                        limit,
                        patchedLimit -> {
                            List<ConfigEntity> configs = db.configHistory().list(
                                    configPath, upstreamBranch, patchedOffset, patchedLimit
                            );
                            if (offsetCommitHash == null && configAtMergeRevision != null) {
                                configs.add(0, configAtMergeRevision);
                            }
                            return configs;
                        }
                )
                .withTotal(() -> {
                    long upstreamCount = db.configHistory().count(configPath, upstreamBranch);
                    return upstreamCount + (configAtMergeRevision != null ? 1 : 0);
                })
                .fetch();
    }

    private Optional<ConfigEntity> getLastValidConfig(
            Path configPath, List<ConfigEntity> configEntities, long offsetCommitNumber, ArcBranch branch
    ) {
        // Если запрос был без офсета, значит мы можем поискать последний валидных конфиг среди последних,
        // однако не факт, что он там будет.
        if (offsetCommitNumber <= 0) {
            Optional<ConfigEntity> configEntity = configEntities.stream()
                    .filter(entity -> entity.getStatus().isValidCiConfig())
                    .findFirst();
            if (configEntity.isPresent()) {
                return configEntity;
            }
        }
        return db.configHistory().findLastValidConfig(configPath, branch);
    }


    private List<FrontendProjectApi.ListProject> toProtoListProjects(
            List<String> projectIds,
            boolean isAllFavorite,
            Set<String> favoriteProjects
    ) {
        Map<String, AbcServiceEntity> services = abcService.getServices(projectIds);

        Map<String, AbcServiceEntity> allServices = abcService.getServices(
                StreamEx.ofValues(services)
                        .flatCollection(AbcServiceEntity::getSlugHierarchy)
                        .toList()
        );

        return projectIds.stream()
                .map(services::get)
                .filter(Objects::nonNull)
                .map(serviceInfo -> toProtoListProject(serviceInfo, isAllFavorite, favoriteProjects, allServices))
                .toList();
    }

    private FrontendProjectApi.ListProject toProtoListProject(
            AbcServiceEntity serviceInfo,
            boolean isAllFavorite,
            Set<String> favoriteProjects,
            Map<String, AbcServiceEntity> allServices
    ) {

        boolean isFavorite = isAllFavorite || favoriteProjects.contains(serviceInfo.getSlug());

        var builder = FrontendProjectApi.ListProject.newBuilder()
                .setProject(ProtoMappers.toProtoProject(serviceInfo))
                .setIsFavorite(isFavorite);

        serviceInfo.getSlugHierarchy().stream()
                .filter(slug -> !slug.equals(serviceInfo.getSlug()))
                .map(slug -> allServices.computeIfAbsent(slug, AbcServiceEntity::empty))
                .map(ProtoMappers::toProtoAbcService)
                .forEach(builder::addAbcHierarchy);

        if (builder.getAbcHierarchyCount() == 0) {
            builder.addAbcHierarchy(ProtoMappers.toProtoAbcService(AbcServiceEntity.root()));
        }
        return builder.build();
    }

    private FrontendProjectApi.ConfigEntity toProtoConfigEntity(ConfigEntity configEntity) {
        ArcCommit commit = arcService.getCommit(configEntity.getRevision());
        return FrontendProjectApi.ConfigEntity.newBuilder()
                .setCommit(ProtoMappers.toProtoCommit(configEntity.getRevision(), commit))
                .setValid(configEntity.getStatus().isValidCiConfig())
                .setHasToken(configEntity.getSecurityState().isValid())
                .setTokenStatus(configEntity.getSecurityState().getValidationStatus().getMessage())
                .addAllProblems(
                        StreamEx.of(configEntity.getProblems())
                                .map(ProjectController::toProtoConfigProblem)
                                .toList()
                )
                .build();
    }

    private static FrontendProjectApi.ConfigProblem toProtoConfigProblem(ConfigProblem configProblem) {
        return FrontendProjectApi.ConfigProblem.newBuilder()
                .setTitle(configProblem.getTitle())
                .setDescription(configProblem.getDescription())
                .setLevel(FrontendProjectApi.ConfigProblem.Level.valueOf(configProblem.getLevel().name()))
                .build();
    }
}
