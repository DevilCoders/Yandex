package ru.yandex.ci.api.controllers.frontend;

import java.nio.file.Path;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.project.ActionConfigState;
import ru.yandex.ci.core.project.ProjectCounters;
import ru.yandex.ci.core.project.ReleaseConfigState;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.launch.auto.AutoReleaseService;
import ru.yandex.ci.engine.launch.auto.AutoReleaseState;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.util.OffsetResults;
import ru.yandex.lang.NonNullApi;

import static java.util.function.Function.identity;
import static java.util.stream.Collectors.mapping;
import static java.util.stream.Collectors.toList;
import static ru.yandex.ci.api.controllers.frontend.ProjectController.TOP_N_BRANCHES_IN_PROJECT;
import static ru.yandex.ci.engine.proto.ProtoMappers.toProtoFlowProcessId;
import static ru.yandex.ci.engine.proto.ProtoMappers.toProtoLaunchStatus;

@Slf4j
@NonNullApi
@RequiredArgsConstructor
class ConfigStateBuilder {

    private final CiMainDb db;
    private final AutoReleaseService autoReleaseService;
    private final BranchService branchService;

    Map<String, List<Common.ConfigState>> toProtoConfigStateByProject(
            List<ConfigState> configStates,
            boolean fillLaunchStatusCounters
    ) {
        var releaseProcessIds = collectReleaseProcessIds(configStates, releaseState -> true);
        var autoReleaseStatesByProject = autoReleaseService.findAutoReleaseStateOrDefault(releaseProcessIds);
        var topBranches = collectTopBranchesByProject(configStates);

        var projectCounters = getProjectCounters(configStates, fillLaunchStatusCounters);

        return configStates.stream()
                .filter(cs -> cs.getProject() != null)
                .collect(Collectors.groupingBy(
                        ConfigState::getProject,
                        LinkedHashMap::new,
                        mapping(cs -> toProtoConfigState(
                                cs,
                                autoReleaseStatesByProject,
                                topBranches,
                                projectCounters.getOrDefault(cs.getProject(), ProjectCounters.empty())
                        ), toList())
                ));
    }

    private Map<String, ProjectCounters> getProjectCounters(
            List<ConfigState> configState,
            boolean fillLaunchStatusCounters
    ) {
        if (!fillLaunchStatusCounters) {
            return Map.of();
        }
        return db.currentOrReadOnly(() ->
                configState.stream()
                        .map(ConfigState::getProject)
                        .filter(Objects::nonNull)
                        .distinct()
                        .collect(Collectors.toMap(
                                identity(),
                                project -> ProjectCounters.create(
                                        db.launches().getActiveReleaseLaunchesCount(project)
                                ))
                        ));
    }

    private Map<CiProcessId, OffsetResults<Branch>> collectTopBranchesByProject(List<ConfigState> configState) {
        var withBranches = collectReleaseProcessIds(configState, ReleaseConfigState::isReleaseBranchesEnabled);
        log.info("Loading top branches for releases for releases {}", withBranches);

        if (withBranches.isEmpty()) {
            return Map.of();
        }

        var cachedTopBranches = db.currentOrReadOnly(
                () -> branchService.getTopBranches(withBranches, TOP_N_BRANCHES_IN_PROJECT + 1)
        );

        var topBranches = new HashMap<CiProcessId, OffsetResults<Branch>>();
        for (var processWithBranches : withBranches) {
            var branchesResult = OffsetResults.builder()
                    .withItems(
                            TOP_N_BRANCHES_IN_PROJECT,
                            limit -> cachedTopBranches.getOrDefault(processWithBranches, List.of())
                    )
                    .fetch();
            topBranches.put(processWithBranches, branchesResult);
        }
        return topBranches;
    }

    private static Common.ConfigState toProtoConfigState(
            ConfigState configState,
            Map<CiProcessId, AutoReleaseState> autoReleaseStates,
            Map<CiProcessId, OffsetResults<Branch>> topBranches,
            ProjectCounters projectCounters
    ) {
        var protoConfigState = Common.ConfigState.newBuilder()
                .setDir(AYamlService.pathToDir(configState.getConfigPath()))
                .setPath(configState.getConfigPath().toString())
                .setTitle(configState.getTitle())
                .setCreated(ProtoConverter.convert(configState.getCreated()))
                .setUpdated(ProtoConverter.convert(configState.getUpdated()));

        var virtualProcessType = ProtoMappers.toVirtualProcessType(
                VirtualCiProcessId.VirtualType.of(configState.getConfigPath()));
        protoConfigState.setVirtualProcessType(virtualProcessType);

        protoConfigState.addAllReleases(
                configState.getReleases()
                        .stream()
                        .map(releaseState -> toProtoReleaseState(
                                configState,
                                autoReleaseStates,
                                topBranches,
                                releaseState,
                                projectCounters
                        ))
                        .toList()
        );

        protoConfigState.addAllFlows(
                getVisibleActions(configState.getActions())
                        .stream()
                        .map(actionState ->
                                toProtoActionState(configState.getConfigPath(), actionState, projectCounters))
                        .toList()
        );

        return protoConfigState.build();
    }

    private static Common.ReleaseState toProtoReleaseState(
            ConfigState configState,
            Map<CiProcessId, AutoReleaseState> autoReleaseStates,
            Map<CiProcessId, OffsetResults<Branch>> topBranches,
            ReleaseConfigState releaseState,
            ProjectCounters projectCounters
    ) {
        var processId = getProcessId(configState, releaseState);
        var autoReleaseState = autoReleaseStates.get(processId);
        return ProtoMappers.toProtoReleaseState(
                processId,
                releaseState,
                Objects.requireNonNull(autoReleaseState),
                topBranches.get(processId),
                projectCounters
        );
    }

    private static List<CiProcessId> collectReleaseProcessIds(
            List<ConfigState> configState,
            Predicate<ReleaseConfigState> releasesFilter
    ) {
        return configState.stream()
                .flatMap(cs -> cs.getReleases().stream()
                        .filter(releasesFilter)
                        .map(release -> getProcessId(cs, release)))
                .toList();
    }

    static List<ActionConfigState> getVisibleActions(List<ActionConfigState> flows) {
        return flows.stream()
                .filter(flowState -> {
                    if (flowState.getShowInActions() != null) {
                        return flowState.getShowInActions();
                    }
                    return !flowState.getTriggers().isEmpty();
                })
                .collect(toList());
    }

    private static Common.FlowState toProtoActionState(
            Path configPath,
            ActionConfigState actionState,
            ProjectCounters projectCounters
    ) {
        var processId = CiProcessId.ofFlow(configPath, actionState.getFlowId());
        var builder = Common.FlowState.newBuilder()
                .setId(toProtoFlowProcessId(processId))
                .setTitle(actionState.getTitle());

        Optional.ofNullable(actionState.getTestId())
                .map(ProtoMappers::toTestId)
                .ifPresent(builder::setTestId);

        Optional.ofNullable(actionState.getDescription()).ifPresent(builder::setDescription);

        projectCounters.getCount(processId)
                .entrySet()
                .stream()
                .map(it -> Common.LaunchStatusCounter.newBuilder()
                        .setStatus(toProtoLaunchStatus(it.getKey()))
                        .setCount(it.getValue().intValue())
                        .build()
                )
                .forEach(builder::addLaunchStatusCounter);
        return builder.build();
    }

    private static CiProcessId getProcessId(ConfigState configState, ReleaseConfigState it) {
        return CiProcessId.ofRelease(configState.getConfigPath(), it.getReleaseId());
    }

}
