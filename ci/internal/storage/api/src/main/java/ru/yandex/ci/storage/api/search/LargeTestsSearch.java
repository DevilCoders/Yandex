package ru.yandex.ci.storage.api.search;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.NavigableMap;
import java.util.Optional;
import java.util.Set;
import java.util.TreeMap;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.kikimr.yql.YqlOrderBy;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlStatementPart;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.storage.api.StorageFrontApi.LargeTestResponse;
import ru.yandex.ci.storage.api.StorageFrontApi.LargeTestStatus;
import ru.yandex.ci.storage.api.StorageFrontApi.ListLargeTestsToolchainsRequest;
import ru.yandex.ci.storage.api.StorageFrontApi.ListLargeTestsToolchainsResponse;
import ru.yandex.ci.storage.api.StorageFrontApi.SearchLargeTestsRequest;
import ru.yandex.ci.storage.api.StorageFrontApi.SearchLargeTestsResponse;
import ru.yandex.ci.storage.core.CheckIteration.IterationType;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.Common.TestStatus;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.DiffSearchFilters;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;

@Slf4j
@AllArgsConstructor
public class LargeTestsSearch {

    private static final Map<LargeTestStatus, Set<CheckStatus>> LARGE_TO_CHECK_MAPPING = Map.of(
            LargeTestStatus.LTS_RUNNING, CheckStatusUtils.ACTIVE,
            LargeTestStatus.LTS_SUCCESS, CheckStatusUtils.SUCCESS,
            LargeTestStatus.LTS_FAILURE, CheckStatusUtils.FAILURE
    );
    private static final Map<CheckStatus, LargeTestStatus> CHECK_TO_LARGE_MAPPING =
            LARGE_TO_CHECK_MAPPING.entrySet().stream()
                    .flatMap(e -> e.getValue().stream().map(s -> Map.entry(s, e.getKey())))
                    .collect(Collectors.toMap(Map.Entry::getKey, Map.Entry::getValue));

    @Nonnull
    private final CiStorageDb db;

    public ListLargeTestsToolchainsResponse listToolchains(ListLargeTestsToolchainsRequest request) {
        return db.currentOrReadOnly(() -> {
            var checkId = CheckEntity.Id.of(request.getCheckId());
            var iterationId = CheckIterationEntity.Id.of(checkId, IterationType.FULL, 0);

            var filters = DiffSearchFilters.builder()
                    .resultTypes(Set.of(Common.ResultType.RT_TEST_SUITE_LARGE));
            // TODO: exclude non-launchable?
            var toolchains = db.testDiffs().listToolchains(iterationId, filters.build());
            log.info("Found {} discovered Large tests toolchains", toolchains.size());

            var response = ListLargeTestsToolchainsResponse.newBuilder();
            for (var toolchain : toolchains) {
                response.addToolchainsBuilder().setToolchain(toolchain);
            }
            return response.build();
        });
    }

    public SearchLargeTestsResponse searchLargeTests(SearchLargeTestsRequest request) {
        return db.currentOrReadOnly(() -> new SingleSearchRequest(request).search());
    }

    private class SingleSearchRequest {
        private final SearchLargeTestsRequest request;
        private final CheckEntity.Id checkId;
        private final LargeResponseResult result = new LargeResponseResult();

        private SingleSearchRequest(@Nonnull SearchLargeTestsRequest request) {
            this.request = request;
            this.checkId = CheckEntity.Id.of(request.getCheckId());
        }

        private SearchLargeTestsResponse search() {

            // Join 2 output:
            //  * Discovered tests from iteration
            //  * Status of launched tasks

            // All registered tasks must be matched against discovered
            collectDiscovered();
            collectRegisteredTasks();

            return buildResponse();
        }

        private SearchLargeTestsResponse buildResponse() {
            var largeTestStatus = request.getLargeTestsStatus();
            var acceptAny = largeTestStatus == LargeTestStatus.LTS_ALL;

            var response = SearchLargeTestsResponse.newBuilder();
            result.merge().forEach((key, value) -> {
                var testDescription = LargeTestResponse.newBuilder();

                boolean acceptLeft;
                if (value.left != null) {
                    testDescription.setLeftStatus(value.left);
                    acceptLeft = acceptAny || largeTestStatus == value.left;
                } else {
                    acceptLeft = false;
                }

                boolean acceptRight;
                if (value.right != null) {
                    testDescription.setRightStatus(value.right);
                    acceptRight = acceptAny || largeTestStatus == value.right;
                } else {
                    acceptRight = false;
                }

                if (acceptLeft || acceptRight) {
                    testDescription.setTestDiff(CheckProtoMappers.toProtoDiffId(value.testDiffId));
                    response.addTests(testDescription);
                }

            });
            return response.build();
        }

        private void collectDiscovered() {
            // Check diffs, similar to large autostart
            var iterationId = CheckIterationEntity.Id.of(checkId, IterationType.FULL, 0);

            var filters = DiffSearchFilters.builder()
                    .resultTypes(Set.of(Common.ResultType.RT_TEST_SUITE_LARGE))
                    .path(request.getPath())
                    .toolchain(request.getToolchain());

            var diffs = db.testDiffs().searchAllDiffs(iterationId, filters.build());
            log.info("Found {} discovered Large tests", diffs.size());

            for (var diff : diffs) {
                if (!diff.isLaunchable()) {
                    continue; // ---
                }
                var key = new TestIdentifier(diff.getId().getPath(), diff.getId().getToolchain());
                var state = new DiscoveredTestState(
                        diff.getId(),
                        state(diff.getLeft()),
                        state(diff.getRight()));

                result.discovered.put(key, state);
            }
        }

        private void collectRegisteredTasks() {
            // Always load discovered tests first - to be sure the discovered list is matched with started

            var parts = new ArrayList<YqlStatementPart<?>>(8);
            parts.add(YqlPredicate.where("id.iterationId.checkId").eq(checkId));
            parts.add(YqlPredicate.where("id.iterationId.iterationType").eq(IterationType.HEAVY.getNumber()));
            parts.add(YqlPredicate.where("type").eq(Common.CheckTaskType.CTT_LARGE_TEST));
            jobNameFilter().ifPresent(parts::add);
            parts.add(YqlOrderBy.orderBy("id"));

            log.info("Searching for executed Large tests: {}", parts);

            var tests = db.checkTasks().find(parts);
            log.info("Found {} executed Large tests", tests.size());

            for (var test : tests) {
                var key = parseTestIdentifier(test);
                if (key == null) {
                    continue; // ---
                }

                var value = result.registered.computeIfAbsent(key, k -> new RegisteredTestState(null, 0, null, 0));

                var isRight = test.isRight();
                var prevIteration = isRight ? value.rightIteration : value.leftIteration;

                // Note: should we ever expect anything like this?
                // Yes, tasks could be restarted, task iteration will not be same as discovery iteration
                if (test.getId().getIterationId().getNumber() <= prevIteration) { // equals? oh my
                    continue; // --
                }

                var status = toLargeTestStatus(test.getStatus());
                if (isRight) {
                    value.right = status;
                } else {
                    value.left = status;
                }
            }
        }

        @Nullable
        private TestIdentifier parseTestIdentifier(CheckTaskEntity test) {
            var job = test.getJobName();

            if (job.contains("@")) { // new job, CiProcessId
                var ciProcessId = CiProcessId.ofString(job);
                var dir = ciProcessId.getDir();
                var subId = ciProcessId.getSubId();
                if (dir.contains("@") && subId.contains("@")) {
                    return new TestIdentifier(dir.split("@", 2)[1], subId.split("@", 2)[0]);
                }
            } else { // old job, path:toolchain
                var pathAndToolchain = job.split(":", 2);
                if (pathAndToolchain.length == 2) {
                    return new TestIdentifier(pathAndToolchain[0], pathAndToolchain[1]);
                }
            }

            log.warn("Unsupported job task name: {}, cannot process {}",
                    test.getJobName(), test.getId());
            return null;
        }

        private Optional<YqlPredicate> jobNameFilter() {
            var path = getPathFilter();
            var toolchain = getToolchainFilter();
            if (path.equals("%") && toolchain.equals("%")) {
                return Optional.empty();
            } else if (path.equals("%") || toolchain.equals("%")) {
                return Optional.of(
                        YqlPredicate.where("jobName").like(path + ":" + toolchain).or(
                                YqlPredicate.where("jobName").like("%@" + path + "/a.yaml:f:" + toolchain + "@%"))
                );
            } else {
                return Optional.of(
                        YqlPredicate.where("jobName").eq(path + ":" + toolchain).or(
                                YqlPredicate.where("jobName").like("%@" + path + "/a.yaml:f:" + toolchain + "@%"))
                );
            }
        }

        private String getPathFilter() {
            String path = request.getPath();
            if (path.isEmpty()) {
                return "%";
            } else {
                if (path.endsWith("*")) {
                    return path.substring(0, path.length() - 1) + "%";
                } else {
                    return path;
                }
            }
        }

        private String getToolchainFilter() {
            if (request.getToolchain().isEmpty()) {
                return "%";
            } else if (request.getToolchain().equals(TestEntity.ALL_TOOLCHAINS)) {
                return "%";
            } else {
                return request.getToolchain();
            }
        }
    }

    @Nullable
    private static LargeTestStatus state(TestStatus testStatus) {
        return testStatus == TestStatus.TS_DISCOVERED
                ? LargeTestStatus.LTS_DISCOVERED
                : null;
    }

    static LargeTestStatus toLargeTestStatus(CheckStatus checkStatus) {
        var status = CHECK_TO_LARGE_MAPPING.get(checkStatus);
        Preconditions.checkState(status != null,
                "Internal error. Unable to map task status: %s", checkStatus);
        return status;
    }

    private static class LargeResponseResult {
        private final NavigableMap<TestIdentifier, DiscoveredTestState> discovered = new TreeMap<>();
        private final Map<TestIdentifier, RegisteredTestState> registered = new HashMap<>();

        NavigableMap<TestIdentifier, DiscoveredTestState> merge() {
            if (registered.isEmpty()) {
                return discovered;
            }
            if (discovered.isEmpty()) {
                return new TreeMap<>();
            }

            registered.forEach((k, v) -> {
                var prev = discovered.get(k);
                if (prev != null) {
                    if (v.left != null) {
                        prev.left = v.left;
                    }
                    if (v.right != null) {
                        prev.right = v.right;
                    }
                }
            });
            return discovered;
        }
    }

    @Value
    private static class TestIdentifier implements Comparable<TestIdentifier> {
        @Nonnull
        String path;

        @Nonnull
        String toolchain;

        @Override
        public int compareTo(@Nonnull TestIdentifier o) {
            var p = path.compareTo(o.path);
            if (p != 0) {
                return p;
            } else {
                return toolchain.compareTo(o.toolchain);
            }
        }
    }

    @AllArgsConstructor
    private static class DiscoveredTestState {
        @Nonnull
        TestDiffEntity.Id testDiffId;
        @Nullable
        LargeTestStatus left;
        @Nullable
        LargeTestStatus right;
    }

    @AllArgsConstructor
    private static class RegisteredTestState {
        @Nullable
        LargeTestStatus left;
        int leftIteration;

        @Nullable
        LargeTestStatus right;
        int rightIteration;
    }
}
