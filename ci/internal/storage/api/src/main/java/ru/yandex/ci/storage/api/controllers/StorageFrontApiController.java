package ru.yandex.ci.storage.api.controllers;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import io.grpc.Context;
import io.grpc.Status;
import io.grpc.stub.StreamObserver;
import lombok.AllArgsConstructor;
import lombok.Value;
import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.util.Strings;

import ru.yandex.ci.client.tvm.grpc.YandexAuthInterceptor;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.api.StorageFrontApiServiceGrpc;
import ru.yandex.ci.storage.api.check.ApiCheckService;
import ru.yandex.ci.storage.api.search.DiffListResult;
import ru.yandex.ci.storage.api.search.LargeTestsSearch;
import ru.yandex.ci.storage.api.search.SearchService;
import ru.yandex.ci.storage.api.util.CheckAttributesCollector;
import ru.yandex.ci.storage.api.util.SuiteSearchToDiffType;
import ru.yandex.ci.storage.api.util.TestRunCommandProvider;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.db.constant.ResultTypeUtils;
import ru.yandex.ci.storage.core.db.constant.TestDiffTypeUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationStatistics;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffStatistics;
import ru.yandex.ci.storage.core.large.LargeStartService;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;

@AllArgsConstructor
public class StorageFrontApiController extends StorageFrontApiServiceGrpc.StorageFrontApiServiceImplBase {
    private final ApiCheckService checkService;
    private final SearchService searchService;
    private final LargeTestsSearch largeTestsSearch;
    private final LargeStartService largeStartService;
    private final StorageCoreCache<?> apiCache;
    private final TestRunCommandProvider testRunCommandProvider;
    private final CheckAttributesCollector checkAttributesCollector;
    private final ArcService arcService;

    @Override
    public void getChecks(
            StorageFrontApi.GetChecksRequest request,
            StreamObserver<StorageFrontApi.GetChecksResponse> responseObserver
    ) {
        if (request.getIdsCount() == 0) {
            throw GrpcUtils.invalidArgumentException("Ids list is empty");
        }

        if (request.getIdsList().stream().anyMatch(String::isEmpty)) {
            throw GrpcUtils.invalidArgumentException("Empty id");
        }

        var checks = new ArrayList<>(checkService.getCheckByTestenvIds(request.getIdsList()));
        if (checks.size() < request.getIdsList().size()) {
            checks.addAll(
                    checkService.getChecks(
                            request.getIdsList().stream()
                                    .map(this::tryParse)
                                    .filter(Objects::nonNull)
                                    .map(CheckEntity.Id::of)
                                    .collect(Collectors.toSet())
                    )
            );
        }

        var checkIds = checks.stream().map(CheckEntity::getId).collect(Collectors.toSet());

        var iterations = checkService.getIterationsByCheckIds(checkIds).stream()
                .collect(Collectors.groupingBy(x -> x.getId().getCheckId()));

        responseObserver.onNext(
                StorageFrontApi.GetChecksResponse.newBuilder()
                        .addAllChecks(
                                checks.stream()
                                        .map(check -> toViewModel(
                                                        check, iterations.getOrDefault(check.getId(), List.of())
                                                )
                                        )
                                        .collect(Collectors.toList())
                        )
                        .build()
        );

        responseObserver.onCompleted();
    }

    @Override
    public void getPostCommitChecks(
            StorageFrontApi.GetPostCommitChecksRequest request,
            StreamObserver<StorageFrontApi.GetPostCommitChecksResponse> responseObserver
    ) {
        var commit = arcService.getCommit(ArcRevision.parse(request.getRevision()));
        if (commit == null) {
            throw GrpcUtils.notFoundException("Commit not found " + request.getRevision());
        }

        var checks = checkService.getPostCommitChecks(commit.getRevision().getCommitId());

        var checkIds = checks.stream().map(CheckEntity::getId).collect(Collectors.toSet());

        var iterations = checkService.getIterationsByCheckIds(checkIds).stream()
                .collect(Collectors.groupingBy(x -> x.getId().getCheckId()));

        responseObserver.onNext(
                StorageFrontApi.GetPostCommitChecksResponse.newBuilder()
                        .addAllChecks(
                                checks.stream()
                                        .map(check -> toViewModel(
                                                        check, iterations.getOrDefault(check.getId(), List.of())
                                                )
                                        )
                                        .collect(Collectors.toList())
                        )
                        .build()
        );

        responseObserver.onCompleted();
    }

    @Override
    public void getRunCommand(
            StorageFrontApi.GetRunCommandRequest request,
            StreamObserver<StorageFrontApi.GetRunCommandResponse> responseObserver
    ) {
        var command = switch (request.getParamCase()) {
            case DIFF_ID -> testRunCommandProvider.getRunCommand(CheckProtoMappers.toDiffId(request.getDiffId()));
            case STATUS_ID -> testRunCommandProvider.getRunCommand(
                    CheckProtoMappers.toTestStatusId(request.getStatusId())
            );
            case PARAM_NOT_SET -> testRunCommandProvider.getRunCommand(CheckProtoMappers.toDiffId(request.getId()));
        };

        responseObserver.onNext(
                StorageFrontApi.GetRunCommandResponse.newBuilder()
                        .setCommand(command)
                        .build()
        );

        responseObserver.onCompleted();
    }

    @Nullable
    private Long tryParse(String value) {
        try {
            return Long.parseLong(value);
        } catch (NumberFormatException ignored) {
            return null;
        }
    }

    private StorageFrontApi.CheckViewModel toViewModel(
            CheckEntity check,
            List<CheckIterationEntity> iterations
    ) {
        var attributes = checkAttributesCollector.fillCheckAttributes(check);
        var iterationViewModels = iterations
                .stream()
                .map(iteration -> toViewModel(check, iteration, TestEntity.ALL_TOOLCHAINS))
                .toList();

        var testenvId = check.getTestenvId();
        if (Strings.isEmpty(testenvId)) {
            testenvId = iterations.stream()
                    .map(CheckIterationEntity::getTestenvId)
                    .filter(Strings::isNotEmpty)
                    .findFirst().orElse("");
        }

        return StorageFrontApi.CheckViewModel
                .newBuilder()
                .setInfoPanel(attributes)
                .setLeftRevision(CheckProtoMappers.toProtoRevision(check.getLeft()))
                .setRightRevision(CheckProtoMappers.toProtoRevision(check.getRight()))
                .setDiffSetId(check.getDiffSetId())
                .setId(check.getId().getId().toString())
                .setOwner(check.getAuthor())
                .addAllTags(check.getTags())
                .setTestenvId(testenvId)
                .setCreated(ProtoConverter.convert(check.getCreated()))
                .setType(CheckOuterClass.CheckType.valueOf(check.getType().name()))
                .setStatus(check.getStatus())
                .setArchiveState(check.getArchiveState())
                .addAllFastIterations(
                        iterationViewModels.stream()
                                .filter(i -> i.getId().getCheckType().equals(CheckIteration.IterationType.FAST))
                                .sorted(Comparator.comparingInt(i -> i.getId().getNumber()))
                                .collect(Collectors.toList())
                )
                .addAllFullIterations(
                        iterationViewModels.stream()
                                .filter(i -> i.getId().getCheckType().equals(CheckIteration.IterationType.FULL))
                                .sorted(Comparator.comparingInt(i -> i.getId().getNumber()))
                                .collect(Collectors.toList())
                )
                .addAllHeavyIterations(
                        iterationViewModels.stream()
                                .filter(i -> i.getId().getCheckType().equals(CheckIteration.IterationType.HEAVY))
                                .sorted(Comparator.comparingInt(i -> i.getId().getNumber()))
                                .collect(Collectors.toList())
                )
                .setLargeTestsState(getLargeTestsState(check, iterations))
                .addAllSuspiciousAlerts(
                        check.getSuspiciousAlerts().stream()
                                .map(alert -> StorageFrontApi.SuspiciousAlert.newBuilder()
                                        .setMessage(alert.getMessage())
                                        .build())
                                .toList()
                )
                .build();
    }

    private StorageFrontApi.LargeTestsState getLargeTestsState(
            CheckEntity check,
            List<CheckIterationEntity> iterations
    ) {
        var firstFullIteration = iterations.stream()
                .filter(iteration -> iteration.getId().isFirstFullIteration())
                .findFirst();

        var testenvId = firstFullIteration
                .map(CheckIterationEntity::getTestenvId)
                .orElse("");

        var largeDiscoveryInProgress = firstFullIteration
                .map(iteration -> iteration.getCheckTaskStatus(Common.CheckTaskType.CTT_LARGE_TEST).isDiscovering())
                .orElse(true);

        return StorageFrontApi.LargeTestsState.newBuilder()
                .setTestenvId(testenvId)
                .setStartAttr(getLargeTestsStartAttribute(check, testenvId))
                .setLargeTestsDiscoveryInProgress(largeDiscoveryInProgress)
                .setRunLargeTestsAfterDiscovery(check.getRunLargeTestsAfterDiscovery())
                .build();

    }

    private StorageFrontApi.LargeTestsStartAttribute getLargeTestsStartAttribute(
            CheckEntity check,
            @Nullable String testenvId
    ) {
        if (!StringUtils.isEmpty(testenvId)) {
            return StorageFrontApi.LargeTestsStartAttribute.LTSA_TESTENV;
        }

        if (!check.getReportStatusToArcanum()) { // TODO: make a complete check?
            return StorageFrontApi.LargeTestsStartAttribute.LTSA_UNSUPPORTED;
        }

        return switch (check.getType()) {
            case BRANCH_PRE_COMMIT, BRANCH_POST_COMMIT -> check.getLargeTestsConfig() == null
                    ? StorageFrontApi.LargeTestsStartAttribute.LTSA_CI_DENY_BRANCH_SETTINGS
                    : StorageFrontApi.LargeTestsStartAttribute.LTSA_CI_ALLOW;
            case TRUNK_PRE_COMMIT, TRUNK_POST_COMMIT -> StorageFrontApi.LargeTestsStartAttribute.LTSA_CI_ALLOW;
            case UNRECOGNIZED -> throw new IllegalStateException("Unsupported check type: " + check.getType());
        };
    }

    private StorageFrontApi.IterationViewModel toViewModel(
            CheckEntity check,
            CheckIterationEntity iteration,
            String toolchain
    ) {
        var attributes = checkAttributesCollector.fillIterationAttributes(check, iteration);

        return StorageFrontApi.IterationViewModel.newBuilder()
                .setId(CheckProtoMappers.toProtoIterationId(iteration.getId()))
                .setStatus(iteration.getStatus())
                .setStatistics(
                        CheckProtoMappers.toProtoStatistics(
                                iteration.getStatistics(), iteration.getStatistics().getToolchain(toolchain)
                        )
                )
                .addToolchains(TestEntity.ALL_TOOLCHAINS)
                .addAllToolchains(
                        iteration.getStatistics().getToolchains().keySet()
                                .stream()
                                .filter(x -> !x.equals(TestEntity.ALL_TOOLCHAINS))
                                .sorted()
                                .collect(Collectors.toList())
                )
                .setCreated(ProtoConverter.convert(iteration.getCreated()))
                .setStart(ProtoConverter.convert(iteration.getStart()))
                .setFinish(ProtoConverter.convert(iteration.getFinish()))
                .setInfo(CheckProtoMappers.toProtoIterationInfo(iteration.getInfo()))
                .setInfoPanel(attributes)
                .addToolchainsStatistics(
                        toToolchainStatistics(iteration.getStatistics().getAllToolchain())
                )
                .addAllToolchainsStatistics(
                        iteration.getStatistics().getToolchains().entrySet()
                                .stream()
                                .filter(x -> !x.getKey().equals(TestEntity.ALL_TOOLCHAINS))
                                .map(x -> toToolchainStatistics(x.getValue()))
                                .sorted(Comparator.comparing(StorageFrontApi.ToolchainViewModel::getFailed).reversed())
                                .collect(Collectors.toList())
                )
                .addAllSuspiciousAlerts(
                        iteration.getSuspiciousAlerts().stream()
                                .map(alert -> StorageFrontApi.SuspiciousAlert.newBuilder()
                                        .setMessage(alert.getMessage())
                                        .build())
                                .toList()
                )
                .build();
    }


    private StorageFrontApi.ToolchainViewModel toToolchainStatistics(
            IterationStatistics.ToolchainStatistics statistics
    ) {
        var main = statistics.getMain().getTotalOrEmpty();
        var extended = statistics.getExtended();
        return StorageFrontApi.ToolchainViewModel.newBuilder()
                .setPassed(main.getPassed())
                .setFailed(main.getFailed())
                .setSkipped(main.getSkipped())
                .setSpecialCases(
                        extended.getFlakyOrEmpty().getTotal() +
                                extended.getTimeoutOrEmpty().getTotal() +
                                extended.getExternalOrEmpty().getTotal() +
                                extended.getInternalOrEmpty().getTotal()
                )
                .build();
    }


    @Override
    public void getIteration(
            StorageFrontApi.GetIterationRequest request,
            StreamObserver<StorageFrontApi.GetIterationResponse> responseObserver
    ) {
        if (request.getId().getCheckId().isEmpty()) {
            throw GrpcUtils.invalidArgumentException("Id is empty");
        }

        var iterationOptional = this.apiCache.iterations().getFresh(CheckProtoMappers.toIterationId(request.getId()));
        if (iterationOptional.isEmpty()) {
            throw GrpcUtils.invalidArgumentException("Iteration not found");
        }

        var iteration = iterationOptional.get();

        var checkOptional = this.apiCache.checks().getFresh(iteration.getId().getCheckId());
        if (checkOptional.isEmpty()) {
            throw GrpcUtils.notFoundException("Check not found");
        }

        responseObserver.onNext(
                StorageFrontApi.GetIterationResponse.newBuilder()
                        .setIteration(toViewModel(checkOptional.get(), iteration, TestEntity.ALL_TOOLCHAINS))
                        .build()
        );

        responseObserver.onCompleted();
    }

    @Override
    public void cancelCheck(
            StorageFrontApi.FrontCancelCheckRequest request,
            StreamObserver<StorageFrontApi.FrontCancelCheckResponse> responseObserver
    ) {
        var checkId = CheckEntity.Id.of(request.getId());
        var check = this.checkService.onUserCancelRequested(checkId, getUsername());
        var iterations = this.checkService.getIterationsByCheckIds(Set.of(checkId));

        responseObserver.onNext(
                StorageFrontApi.FrontCancelCheckResponse.newBuilder()
                        .setCheck(toViewModel(check, iterations))
                        .build()
        );

        responseObserver.onCompleted();
    }

    @Override
    public void searchSuites(
            StorageFrontApi.SearchSuitesRequest request,
            StreamObserver<StorageFrontApi.SearchSuitesResponse> responseObserver
    ) {
        if (request.getId().getCheckId().isEmpty()) {
            throw GrpcUtils.invalidArgumentException("Id is empty");
        }

        if (request.getSearch().getToolchain().isEmpty()) {
            throw GrpcUtils.invalidArgumentException("Toolchain is empty");
        }

        var iterationOptional = this.apiCache.iterations().getFresh(CheckProtoMappers.toIterationId(request.getId()));
        if (iterationOptional.isEmpty()) {
            throw GrpcUtils.notFoundException("Iteration not found");
        }

        var iteration = iterationOptional.get();

        var checkOptional = this.apiCache.checks().getFresh(iteration.getId().getCheckId());
        if (checkOptional.isEmpty()) {
            throw GrpcUtils.notFoundException("Check not found");
        }

        var filters = CheckProtoMappers.toSearchFilters(request.getSearch(), request.getPageId());
        var searchResult = this.searchService.searchSuites(
                iteration.getId(),
                filters,
                iteration.isUseImportantDiffIndex()
        );

        responseObserver.onNext(
                StorageFrontApi.SearchSuitesResponse.newBuilder()
                        .setIteration(toViewModel(checkOptional.get(), iteration, request.getSearch().getToolchain()))
                        .addAllSuites(
                                searchResult.getSuites().stream()
                                        .map(s -> this.getDiffViewModel(s, request.getSearch(), false))
                                        .collect(Collectors.toList())
                        )
                        .setPaging(CheckProtoMappers.toSuitePaging(searchResult.getPageCursor()))
                        .addAllDiffTypes(SuiteSearchToDiffType.convert(filters))
                        .build()
        );

        responseObserver.onCompleted();
    }

    @Override
    public void listSuite(
            StorageFrontApi.ListSuiteRequest request,
            StreamObserver<StorageFrontApi.ListSuiteResponse> responseObserver
    ) {
        if (request.getDiffId().getTestId().getSuiteId().isEmpty()) {
            throw GrpcUtils.invalidArgumentException("Diff id are empty");
        }

        var iterationOptional = this.apiCache.iterations().getFresh(
                CheckProtoMappers.toIterationId(request.getDiffId().getIterationId())
        );

        if (iterationOptional.isEmpty()) {
            throw GrpcUtils.notFoundException("Iteration not found");
        }

        var iteration = iterationOptional.get();

        var searchResult = this.searchService.listSuite(
                CheckProtoMappers.toDiffId(request.getDiffId()),
                CheckProtoMappers.toSearchFilters(request.getSearch()),
                request.getPage(),
                iteration.isUseSuiteDiffIndex()
        );

        responseObserver.onNext(
                StorageFrontApi.ListSuiteResponse.newBuilder()
                        .addAllChildren(
                                searchResult.getDiffs().stream()
                                        .map(x -> this.getDiffViewModel(x, request.getSearch(), true))
                                        .collect(Collectors.toList())
                        )
                        .setHasMore(searchResult.isHasMore())
                        .build()
        );

        responseObserver.onCompleted();
    }

    @Override
    public void getDiffDetails(
            StorageFrontApi.GetDiffDetailsRequest request,
            StreamObserver<StorageFrontApi.GetDiffDetailsResponse> responseObserver
    ) {
        if (request.getId().getIterationId().getCheckId().isEmpty()) {
            throw GrpcUtils.invalidArgumentException("Iteration id is empty");
        }

        if (
                request.getId().getTestId().getSuiteId().isEmpty() ||
                        request.getId().getTestId().getToolchain().isEmpty()
        ) {
            throw GrpcUtils.invalidArgumentException("Test id is empty");
        }

        var searchResult = searchService.listDiff(
                CheckProtoMappers.toDiffId(request.getId())
        );

        var children = searchResult.getChildren().stream()
                .collect(Collectors.groupingBy(x -> TestDiffTypeUtils.isAddedFailure(x.getDiff().getDiffType())));

        var response = StorageFrontApi.GetDiffDetailsResponse.newBuilder();
        response.addAllChildren(toDiffDetails(request, searchResult, children.get(true)));
        response.addAllChildren(toDiffDetails(request, searchResult, children.get(false)));

        responseObserver.onNext(response.build());

        responseObserver.onCompleted();
    }

    private List<StorageFrontApi.DiffDetailsViewModel> toDiffDetails(
            StorageFrontApi.GetDiffDetailsRequest request,
            DiffListResult searchResult,
            @Nullable List<DiffListResult.DiffWithRuns> diffs
    ) {
        if (diffs == null) {
            return List.of();
        }

        return diffs.stream().map(
                        diff -> getDiffDetailsViewModel(
                                diff, request.getSearch(), searchResult.getDiff().getDiffType()
                        )
                )
                .sorted(Comparator.comparing(
                        x -> x.getDiff().getId().getTestId().getToolchain()
                ))
                .collect(Collectors.toList());
    }

    private StorageFrontApi.DiffDetailsViewModel getDiffDetailsViewModel(
            DiffListResult.DiffWithRuns diff,
            StorageFrontApi.SuiteSearch search,
            Common.TestDiffType parentDiffType
    ) {
        var diffViewModel = getDiffViewModel(diff.getDiff(), search, true);
        return StorageFrontApi.DiffDetailsViewModel.newBuilder()
                .setDiff(diffViewModel)
                .setHighlight(diff.getDiff().getDiffType().equals(parentDiffType))
                .addAllTestRuns(
                        diff.getRuns().stream()
                                .map(CheckProtoMappers::toProtoTestRun)
                                .collect(Collectors.toList())
                )
                .build();
    }


    @Override
    public void searchDiffs(
            StorageFrontApi.SearchDiffsRequest request,
            StreamObserver<StorageFrontApi.SearchDiffsResponse> responseObserver
    ) {
        if (request.getIterationId().getCheckId().isEmpty()) {
            throw GrpcUtils.invalidArgumentException("Id is empty");
        }

        var iterationOptional = this.apiCache.iterations().getFresh(
                CheckProtoMappers.toIterationId(request.getIterationId())
        );
        if (iterationOptional.isEmpty()) {
            throw GrpcUtils.notFoundException("Iteration not found");
        }

        var iteration = iterationOptional.get();

        var checkOptional = this.apiCache.checks().getFresh(iteration.getId().getCheckId());
        if (checkOptional.isEmpty()) {
            throw GrpcUtils.notFoundException("Check not found");
        }

        var searchResult = this.searchService.searchDiffs(
                iteration.getId(),
                CheckProtoMappers.toSearchFilters(request.getSearch(), request.getPageId())
        );

        responseObserver.onNext(
                StorageFrontApi.SearchDiffsResponse.newBuilder()
                        .setIteration(toViewModel(checkOptional.get(), iteration, request.getSearch().getToolchain()))
                        .addAllDiffs(
                                searchResult.getDiffs().stream()
                                        .map(s -> this.getDiffViewModel(s, null, false))
                                        .collect(Collectors.toList())
                        )
                        .setPaging(
                                CheckProtoMappers.toDiffPaging(searchResult.getPageCursor())
                        )
                        .build()
        );

        responseObserver.onCompleted();
    }

    @Override
    public void listLargeTestsToolchains(
            StorageFrontApi.ListLargeTestsToolchainsRequest request,
            StreamObserver<StorageFrontApi.ListLargeTestsToolchainsResponse> responseObserver) {
        if (request.getCheckId().isEmpty()) {
            throw GrpcUtils.invalidArgumentException("Check Id is empty");
        }

        responseObserver.onNext(largeTestsSearch.listToolchains(request));
        responseObserver.onCompleted();
    }

    @Override
    public void searchLargeTests(
            StorageFrontApi.SearchLargeTestsRequest request,
            StreamObserver<StorageFrontApi.SearchLargeTestsResponse> responseObserver
    ) {
        if (request.getCheckId().isEmpty()) {
            throw GrpcUtils.invalidArgumentException("Check Id is empty");
        }

        responseObserver.onNext(largeTestsSearch.searchLargeTests(request));
        responseObserver.onCompleted();
    }

    @Override
    public void startLargeTests(
            StorageFrontApi.StartLargeTestsRequest request,
            StreamObserver<StorageFrontApi.StartLargeTestsResponse> responseObserver
    ) {
        Context.current().fork().run(() -> startLargeTestsInternal(request, responseObserver));
    }

    @Override
    public void scheduleLargeTests(
            StorageFrontApi.ScheduleLargeTestsRequest request,
            StreamObserver<StorageFrontApi.ScheduleLargeTestsResponse> responseObserver
    ) {
        Context.current().fork().run(() -> scheduleLargeTestsInternal(request, responseObserver));
    }

    private void scheduleLargeTestsInternal(
            StorageFrontApi.ScheduleLargeTestsRequest request,
            StreamObserver<StorageFrontApi.ScheduleLargeTestsResponse> responseObserver
    ) {
        largeStartService.scheduleLargeTestsManual(
                CheckEntity.Id.of(request.getCheckId()),
                request.getRunLargeTestsAfterDiscovery(),
                getUsername()
        );
        responseObserver.onNext(StorageFrontApi.ScheduleLargeTestsResponse.getDefaultInstance());
        responseObserver.onCompleted();
    }

    private void startLargeTestsInternal(
            StorageFrontApi.StartLargeTestsRequest request,
            StreamObserver<StorageFrontApi.StartLargeTestsResponse> responseObserver
    ) {
        var username = getUsername();

        var tests = request.getTestDiffsList().stream()
                .map(CheckProtoMappers::toDiffId)
                .collect(Collectors.toList());

        largeStartService.startLargeTestsManual(tests, username);

        responseObserver.onNext(StorageFrontApi.StartLargeTestsResponse.getDefaultInstance());
        responseObserver.onCompleted();
    }

    private StorageFrontApi.DiffViewModel getDiffViewModel(
            TestDiffEntity entity,
            @Nullable StorageFrontApi.SuiteSearch search,
            boolean useSelfStatistics
    ) {
        return StorageFrontApi.DiffViewModel.newBuilder()
                .setId(CheckProtoMappers.toProtoDiffId(entity.getId()))
                .setDiffType(entity.getDiffType())
                .setType(entity.getId().getResultType())
                .setName(entity.getName())
                .setSubtestName(entity.getSubtestName())
                .setIsMuted(entity.isMuted())
                .setIsStrongMode(entity.isStrongMode())
                .setStrongModeAYaml(entity.getStrongModeAYaml())
                .addAllTags(entity.getTagsList())
                .setLeft(entity.getLeft())
                .setRight(entity.getRight())
                .setIsSuite(ResultTypeUtils.isSuite(entity.getId().getResultType()))
                .setNumberOfChildren(
                        search == null ? 0 : getNumberOfDiffs(entity, search, useSelfStatistics)
                )
                .setStatistics(
                        ResultTypeUtils.isSuite(entity.getId().getResultType()) ?
                                getStatistics(entity.getStatistics().getChildren()) :
                                getStatistics(entity.getStatistics().getSelf())
                )
                .build();
    }

    @Override
    public void getSuggest(
            StorageFrontApi.GetSuggestRequest request,
            StreamObserver<StorageFrontApi.GetSuggestResponse> responseObserver
    ) {
        var results = this.searchService.getSuggest(
                CheckProtoMappers.toIterationId(request.getIterationId()),
                request.getEntityType(),
                request.getValue()
        );

        responseObserver.onNext(
                StorageFrontApi.GetSuggestResponse.newBuilder()
                        .addAllResults(results)
                        .build()
        );

        responseObserver.onCompleted();
    }

    private StorageFrontApi.DiffStatistics getStatistics(TestDiffStatistics.StatisticsGroup statistics) {
        var stage = statistics.getStage();
        var extended = statistics.getExtended();

        var passed = stage.getPassed();

        var failed = stage.getFailed() + stage.getFailedByDeps()
                + extended.getExternalOrEmpty().getTotal() - extended.getExternalOrEmpty().getPassedAdded();

        var skipped = stage.getSkipped();
        var flaky = extended.getFlakyOrEmpty().getTotal() - extended.getFlakyOrEmpty().getPassedAdded();
        var timeout = extended.getTimeoutOrEmpty().getTotal() - extended.getTimeoutOrEmpty().getPassedAdded();
        var internal = extended.getInternalOrEmpty().getTotal() - extended.getInternalOrEmpty().getPassedAdded();
        var deleted = extended.getDeletedOrEmpty().getTotal();

        return StorageFrontApi.DiffStatistics.newBuilder()
                .setPassed(passed)
                .setFailed(failed)
                .setSkipped(skipped)
                .setFlaky(flaky)
                .setTimeout(timeout)
                .setInternal(internal)
                .setDeleted(deleted)
                .build();
    }

    private int getNumberOfDiffs(
            TestDiffEntity entity, StorageFrontApi.SuiteSearch search, boolean useSelfStatistics
    ) {
        var statistics = entity.getStatistics();
        if (!search.getSpecialCases().equals(StorageFrontApi.SpecialCasesFilter.SPECIAL_CASE_NONE)) {
            return getNumberOfDiffsForSpecialCases(entity, search, useSelfStatistics);
        }

        var onlyChanged = search.getCategory().equals(StorageFrontApi.CategoryFilter.CATEGORY_CHANGED);
        var stage = useSelfStatistics ?
                statistics.getSelf().getStage() :
                statistics.getChildren().getStage();

        return switch (search.getStatusFilter()) {
            case STATUS_ALL -> onlyChanged ?
                    stage.getFailedAdded() + stage.getPassedAdded() + stage.getSkippedAdded() :
                    stage.getTotal();

            case STATUS_PASSED -> onlyChanged ? stage.getPassedAdded() : stage.getPassed();
            case STATUS_FAILED -> onlyChanged ? stage.getFailedAdded() : stage.getFailed();
            case STATUS_FAILED_WITH_DEPS -> onlyChanged ?
                    stage.getFailedAdded() + stage.getFailedByDepsAdded() :
                    stage.getFailed() + stage.getFailedByDeps();
            case STATUS_SKIPPED -> onlyChanged ? stage.getSkippedAdded() : stage.getSkipped();
            case UNRECOGNIZED -> throw new RuntimeException();
        };
    }

    private int getNumberOfDiffsForSpecialCases(
            TestDiffEntity entity, StorageFrontApi.SuiteSearch search, boolean useSelfStatistics
    ) {
        var statistics = entity.getStatistics();
        var extended = useSelfStatistics ?
                statistics.getSelf().getExtended() :
                statistics.getChildren().getExtended();

        var stage = useSelfStatistics ?
                statistics.getSelf().getStage() :
                statistics.getChildren().getStage();

        var fields = switch (search.getSpecialCases()) {
            case SPECIAL_CASE_EXTERNAL -> new SpecialCaseFields(extended.getExternalOrEmpty());
            case SPECIAL_CASE_INTERNAL -> new SpecialCaseFields(extended.getInternalOrEmpty());
            case SPECIAL_CASE_TIMEOUT -> new SpecialCaseFields(extended.getTimeoutOrEmpty());
            case SPECIAL_CASE_MUTED -> new SpecialCaseFields(extended.getMutedOrEmpty());
            case SPECIAL_CASE_FLAKY -> new SpecialCaseFields(extended.getFlakyOrEmpty());
            case SPECIAL_CASE_ADDED -> new SpecialCaseFields(extended.getAddedOrEmpty());
            case SPECIAL_CASE_DELETED -> new SpecialCaseFields(extended.getDeletedOrEmpty());
            case SPECIAL_CASE_BROKEN_BY_DEPS -> new SpecialCaseFields(
                    stage.getFailedByDeps(), stage.getPassedByDepsAdded(), stage.getFailedByDepsAdded()
            );
            case SPECIAL_CASE_FAILED_IN_STRONG_MODE -> new SpecialCaseFields(
                    stage.getFailedInStrongMode(), stage.getFailedInStrongMode(), 0
            );
            case UNRECOGNIZED, SPECIAL_CASE_NONE -> throw new RuntimeException();
        };

        var category = search.getCategory();
        if (search.getStatusFilter().equals(StorageFrontApi.StatusFilter.STATUS_ALL)) {
            if (category.equals(StorageFrontApi.CategoryFilter.CATEGORY_ALL)) {
                return fields.getTotal();
            } else {
                return fields.failedAdded + fields.passedAdded;
            }
        }

        if (search.getStatusFilter().equals(StorageFrontApi.StatusFilter.STATUS_FAILED)) {
            if (category.equals(StorageFrontApi.CategoryFilter.CATEGORY_ALL)) {
                return fields.failedAdded;
            } else {
                return fields.failedAdded + fields.passedAdded;
            }
        }

        if (search.getStatusFilter().equals(StorageFrontApi.StatusFilter.STATUS_PASSED)) {
            return fields.passedAdded;
        }

        return -1;
    }

    private static String getUsername() {
        return YandexAuthInterceptor.getAuthenticatedUser()
                .orElseThrow(Status.UNAUTHENTICATED::asRuntimeException)
                .getLogin();
    }

    @Value
    @AllArgsConstructor
    private static class SpecialCaseFields {
        int total;
        int passedAdded;
        int failedAdded;

        private SpecialCaseFields(ExtendedStageStatistics statistics) {
            this.total = statistics.getTotal();
            this.passedAdded = statistics.getPassedAdded();
            this.failedAdded = statistics.getFailedAdded();
        }
    }
}
