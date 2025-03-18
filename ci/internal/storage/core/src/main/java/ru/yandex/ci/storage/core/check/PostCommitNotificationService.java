package ru.yandex.ci.storage.core.check;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import com.google.common.primitives.UnsignedLongs;
import lombok.AllArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.core.CiBadgeEvents;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.constant.ResultTypeUtils;
import ru.yandex.ci.storage.core.db.constant.TestDiffTypeUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.groups.GroupEntity;
import ru.yandex.ci.storage.core.db.model.revision.RevisionEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.DiffSearchFilters;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsSender;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;

@SuppressWarnings("UnstableApiUsage")
@Slf4j
@AllArgsConstructor
public class PostCommitNotificationService {
    private static final int ROW_LIMIT = 100 * 1000;
    private static final int PATH_LIMIT = 10;
    private static final int TESTS_IN_PATH_LIMIT = 10;

    private final CiStorageDb db;
    private final BadgeEventsSender badgeEventsSender;

    public void process(CheckIterationEntity.Id iterationId) {
        var iteration = db.currentOrReadOnly(() -> db.checkIterations().get(iterationId));
        process(iteration);
    }

    private void process(CheckIterationEntity iteration) {
        var statistics = iteration.getStatistics().getAllToolchain();

        log.info(
                "Processing iteration: {}, status: {}, statistics: {}",
                iteration.getId(), iteration.getStatus(), statistics
        );

        if (
                statistics.getMain().getTotalOrEmpty().getFailedAdded() == 0 &&
                        statistics.getMain().getTotalOrEmpty().getPassedAdded() == 0
        ) {
            log.info("No new passed or failed tests");
        } else {
            processChanged(iteration);
        }
    }

    private void processChanged(CheckIterationEntity iteration) {
        var check = db.currentOrReadOnly(() -> db.checks().get(iteration.getId().getCheckId()));
        var revision = db.currentOrReadOnly(
                () -> db.revisions().get(new RevisionEntity.Id(check.getRight().getRevisionNumber(),
                        check.getRight().getBranch()))
        );

        var diffs = db.scan().withMaxSize(ROW_LIMIT).run(
                () -> db.testDiffs().searchDiffs(
                        iteration.getId(),
                        DiffSearchFilters.builder()
                                .diffTypes(TestDiffTypeUtils.NOTIFIED)
                                .toolchain(TestEntity.ALL_TOOLCHAINS)
                                .notificationFilter(StorageFrontApi.NotificationFilter.NF_NOT_MUTED)
                                .build(),
                        0,
                        ROW_LIMIT
                )
        );

        var groups = db.currentOrReadOnly(
                () -> db.groups().readTable().collect(
                        Collectors.toMap(x -> x.getId().getName(), GroupEntity::getUsers)
                )
        );

        if (diffs.size() >= ROW_LIMIT) {
            log.warn("Row limit reached {}", diffs.size());
        }

        log.info("Found {} diffs", diffs.size());

        if (diffs.isEmpty()) {
            return;
        }

        var notificationsByOwner = new HashMap<String, TestStatusNotification>();

        for (var diff : diffs) {
            var owners = new ArrayList<>(diff.getOwners().getLogins());
            diff.getOwners().getGroups().forEach(group -> owners.addAll(groups.getOrDefault(group, Set.of())));

            for (var owner : owners) {
                if (owner.equals(check.getAuthor())) {
                    continue;
                }

                if (!notificationsByOwner.containsKey(owner)) {
                    notificationsByOwner.put(owner, new TestStatusNotification(owner, new ArrayList<>()));
                }
                notificationsByOwner.get(owner).diffs.add(diff);
            }
        }

        log.info("Prepared {} notifications", notificationsByOwner.size());

        var messages = notificationsByOwner.values().stream()
                .map(x -> convert(check, iteration, revision, x))
                .map(m -> CiBadgeEvents.Event.newBuilder().setTestsChangedEvent(m).build())
                .toList();

        log.info("Prepared {} messaged", messages.size());

        messages.forEach(m -> badgeEventsSender.sendEvent(check.getId(), m));

        badgeEventsSender.sendEvent(
                check.getId(),
                CiBadgeEvents.Event.newBuilder()
                        .setYouBrokeTestsEvent(
                                convert(
                                        check, iteration, revision,
                                        new TestStatusNotification(check.getAuthor(), diffs)
                                )
                        )
                        .build()
        );
    }

    private CiBadgeEvents.TestsChangedEvent convert(
            CheckEntity check,
            CheckIterationEntity iteration,
            RevisionEntity revision,
            TestStatusNotification notification
    ) {
        var byPath = notification.diffs.stream().collect(Collectors.groupingBy(x -> x.getId().getPath()));
        var splitByStatus = notification.diffs.stream().collect(
                Collectors.groupingBy(
                        x -> x.getRight() == Common.TestStatus.TS_OK || x.getRight() == Common.TestStatus.TS_XFAILED
                )
        );

        return CiBadgeEvents.TestsChangedEvent.newBuilder()
                .setCheckId(check.getId().getId())
                .setAuthor(check.getAuthor())
                .setLeftRevision(CheckProtoMappers.toProtoRevision(check.getLeft()))
                .setRightRevision(CheckProtoMappers.toProtoRevision(check.getRight()))
                .setRightRevisionTitle(revision.getMessage())
                .setOwner(notification.owner)
                .setIterationType(iteration.getId().getIterationType())
                .setNumberOfPaths(byPath.size())
                .addAllPaths(
                        byPath.entrySet().stream()
                                .limit(PATH_LIMIT)
                                .map(x -> convert(x.getKey(), x.getValue()))
                                .toList()

                )
                .setNumberOfFixedTests(splitByStatus.getOrDefault(true, List.of()).size())
                .setNumberOfBrokenTests(splitByStatus.getOrDefault(false, List.of()).size())
                .build();
    }

    private CiBadgeEvents.TestPathInfo convert(String path, List<TestDiffEntity> diffs) {
        var byTestId = diffs.stream().collect(Collectors.groupingBy(x -> x.getId().getTestId()));
        return CiBadgeEvents.TestPathInfo.newBuilder()
                .setPath(path)
                .setNumberOfTests(diffs.size())
                .addAllTests(
                        byTestId.values().stream()
                                .limit(TESTS_IN_PATH_LIMIT)
                                .map(this::convert)
                                .toList()
                )
                .build();
    }

    private CiBadgeEvents.TestInfo convert(List<TestDiffEntity> diffs) {
        var first = diffs.get(0);

        var results = db.currentOrReadOnly(
                () -> db.testResults().find(
                        first.getId().getCheckId(),
                        first.getId().getIterationId().getIterationType(),
                        first.getId().getSuiteId(),
                        first.getId().getToolchain(),
                        first.getId().getTestId()
                )
        );

        var linkNames = new ArrayList<String>();
        var linkValues = new ArrayList<String>();
        if (!results.isEmpty()) {
            results.get(0).getLinks().forEach((key, value) -> value.forEach(link -> {
                        linkNames.add(key);
                        linkValues.add(link);
                    }
            ));
        }

        return CiBadgeEvents.TestInfo.newBuilder()
                .setHid(UnsignedLongs.toString(first.getId().getTestId()))
                .setSuiteHid(UnsignedLongs.toString(first.getId().getSuiteId()))
                .setName(first.getName())
                .setSubtestName(first.getSubtestName())
                .setResultType(first.getId().getResultType())
                .addAllToolchains(
                        diffs.stream().map(
                                diff -> CiBadgeEvents.ToolchainInfo.newBuilder()
                                        .setName(diff.getId().getToolchain())
                                        .setLeft(diff.getLeft())
                                        .setRight(diff.getRight())
                                        .setDiffType(diff.getDiffType())
                                        .build()
                        ).toList()
                )
                .addAllTags(first.getTagsList())
                .addAllLinkNames(linkNames)
                .addAllLinkValues(linkValues)
                .setOldTestId(
                        ResultTypeUtils.isSuite(first.getId().getResultType()) ?
                                first.getOldSuiteId() : first.getOldTestId()
                )
                .build();
    }

    @Value
    private static class TestStatusNotification {
        String owner;
        List<TestDiffEntity> diffs;
    }

}
