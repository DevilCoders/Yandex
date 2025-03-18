package ru.yandex.ci.storage.core.large;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import com.google.common.hash.HashCode;
import com.google.common.hash.HashFunction;
import com.google.common.hash.Hashing;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.constant.StorageLimits;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.DiffSearchFilters;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.large.LargeAutostartMatchers.LargeTestMatcher;
import ru.yandex.ci.storage.core.large.LargeAutostartMatchers.NativeBuildAutostartMatcher;
import ru.yandex.ci.storage.core.large.LargeStartService.TestDiffWithPath;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Slf4j
@RequiredArgsConstructor
public class LargeAutostartService {

    @Nonnull
    private final CiStorageDb db;

    @Nonnull
    private final AutocheckTaskScheduler autocheckTaskScheduler;

    @Nonnull
    private final LargeAutostartMatchers largeAutostartMatchers;

    @Nonnull
    private final BazingaTaskManager bazingaTaskManager;

    public void tryAutostart(@Nonnull CheckIterationEntity.Id iterationId) {
        log.info("Processing tests auto-start {}", iterationId);

        var checkId = iterationId.getCheckId();
        var check = db.currentOrReadOnly(() -> db.checks().get(checkId));

        var largeTestsMatchers = largeAutostartMatchers.collectLargeTestsMatchers(check);
        var nativeBuildsMatchers = largeAutostartMatchers.collectNativeBuildsMatchers(check);

        var matched = List.of(
                new LargeTestsMatcher(iterationId, largeTestsMatchers).matchAutostart(),
                new NativeBuildsMatcher(iterationId, nativeBuildsMatchers).matchAutostart()
        );

        var anyStarted = false;
        for (var match : matched) {
            anyStarted = autocheckTaskScheduler.trySchedule(iterationId, match.checkTaskType, match.diffs) ||
                    anyStarted;
        }

        if (!anyStarted) {
            markDiscoveredCommit(check);
        }
    }

    private void markDiscoveredCommit(CheckEntity check) {
        if (check.getType() == CheckOuterClass.CheckType.TRUNK_POST_COMMIT) {
            log.info("Schedule marking commit as discovered: {}", check.getRight().getRevision());
            bazingaTaskManager.schedule(new MarkDiscoveredCommitTask(check.getId()));
        }
    }

    @RequiredArgsConstructor
    private class LargeTestsMatcher {
        private final CheckIterationEntity.Id iterationId;
        private final List<? extends LargeTestMatcher> matchers;

        MatchedDiffs matchAutostart() {
            log.info("Total {} rules to match: {}", matchers.size(), matchers);
            if (matchers.isEmpty()) {
                // No need to select anything
                return new MatchedDiffs(Common.CheckTaskType.CTT_LARGE_TEST, List.of());
            }

            var diffs = loadDiffs();

            log.info("Loaded {} records matched for this check", diffs.size());
            diffs.forEach(diff -> log.info("path=[{}], toolchain=[{}]",
                    diff.getId().getPath(), diff.getId().getToolchain()));

            var autostartDiffs = matchAutostartDiffs(diffs);
            log.info("Matched {} Large tests for autostart", autostartDiffs.size());

            return new MatchedDiffs(Common.CheckTaskType.CTT_LARGE_TEST, autostartDiffs);
        }

        private List<TestDiffEntity> loadDiffs() {
            var filters = DiffSearchFilters.builder()
                    .resultTypes(Set.of(Common.ResultType.RT_TEST_SUITE_LARGE))
                    .build();

            return db.scan()
                    .withMaxSize(StorageLimits.MAX_LARGE_TESTS)
                    .run(() -> db.testDiffs().searchAllDiffs(iterationId, filters));
        }

        private List<TestDiffWithPath> matchAutostartDiffs(List<TestDiffEntity> diffs) {
            List<TestDiffWithPath> matched = new ArrayList<>(diffs.size());
            for (var diff : diffs) {
                if (!diff.isLaunchable()) {
                    continue; // --- Not accepted
                }

                for (var matcher : matchers) { // TODO: optimize?
                    if (matcher.isAccepted(diff.getId())) {
                        matched.add(new TestDiffWithPath(diff, matcher.getConfigPath(), null));
                        break;
                    }
                }
            }
            return matched;
        }
    }

    @RequiredArgsConstructor
    private class NativeBuildsMatcher {
        private final CheckIterationEntity.Id iterationId;
        private final List<NativeBuildAutostartMatcher> matchers;

        MatchedDiffs matchAutostart() {
            log.info("Total {} Native builds to auto-start: {}", matchers.size(), matchers);

            var diffs = loadDiffs();

            log.info("Loaded {} records matched for this check", diffs.size());
            diffs.forEach(diff -> log.info("path=[{}]",
                    diff.getId().getPath()));

            var autostartDiffs = matchAutostartDiffs(diffs);
            log.info("Matched {} Native builds for autostart", autostartDiffs.size());

            return new MatchedDiffs(Common.CheckTaskType.CTT_NATIVE_BUILD, autostartDiffs);
        }

        private List<TestDiffEntity> loadDiffs() {
            if (matchers.isEmpty()) {
                return List.of(); // No need to select anything
            }

            var allTargets = matchers.stream()
                    .flatMap(b -> b.getMatchTargets().stream())
                    .collect(Collectors.toSet());

            var filters = DiffSearchFilters.builder()
                    .pathIn(allTargets)
                    .resultTypes(Set.of(Common.ResultType.RT_BUILD))
                    .build();

            return db.scan()
                    .withMaxSize(StorageLimits.MAX_LARGE_TESTS)
                    .run(() -> db.testDiffs().searchAllDiffs(iterationId, filters));
        }

        private List<TestDiffWithPath> matchAutostartDiffs(List<TestDiffEntity> diffs) {
            // test diff entities will be converted in a way to be used by default large tests start

            for (var diff : diffs) {
                for (var matcher : matchers) {
                    matcher.checkAccepted(diff);
                }
            }

            return matchers.stream()
                    .filter(matcher -> !matcher.getDiffs().isEmpty())
                    .map(matcher -> {
                        var newPath = matcher.getDiffs().stream()
                                .map(TestDiffEntity::getId)
                                .map(TestDiffEntity.Id::getPath)
                                .collect(Collectors.joining(";"));

                        var diff = matcher.getDiffs().get(0);

                        var newId = diff.getId().toBuilder()
                                .path(newPath) // todo: remove this madness https://st.yandex-team.ru/CI-4123
                                .toolchain(matcher.getToolchain())
                                .suiteId(hash(Hashing.sipHash24(), matcher).asLong())
                                .build();

                        var newDiff = diff.toBuilder()
                                .id(newId)
                                .oldSuiteId(hash(Hashing.md5(), matcher).toString())
                                .build();
                        return new TestDiffWithPath(newDiff, matcher.getPath(), matcher.getDir());
                    })
                    .toList();
        }

        private static HashCode hash(HashFunction hashFunction, NativeBuildAutostartMatcher matcher) {
            return hashFunction.newHasher()
                    .putString(matcher.getDir(), StandardCharsets.UTF_8)
                    .putString(matcher.getToolchain(), StandardCharsets.UTF_8)
                    .putString(matcher.getMatchTargets().toString(), StandardCharsets.UTF_8)
                    .hash();
        }
    }

    @Value
    private static class MatchedDiffs {
        @Nonnull
        Common.CheckTaskType checkTaskType;
        @Nonnull
        List<TestDiffWithPath> diffs;
    }
}
