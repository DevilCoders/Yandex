package ru.yandex.ci.storage.tms.services;

import java.time.Instant;
import java.util.List;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.primitives.UnsignedLongs;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.db.model.KeyValue;
import ru.yandex.ci.storage.core.CiBadgeEvents;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.test_mute.TestMuteEntity;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsSender;

@SuppressWarnings("UnstableApiUsage")
@Slf4j
@AllArgsConstructor
public class MuteDigestService {
    public static final KeyValue.Id LAST_NOTIFICATION_KEY = KeyValue.Id.of("storage", "mute_digest_last_notification");

    private static final int PATH_LIMIT = 20;

    private static final int TESTS_IN_PATH_LIMIT = 10;

    private final CiStorageDb db;
    private final BadgeEventsSender badgeEventsSender;

    public void execute(int batchSize) {
        while (executeLimited(batchSize) == batchSize) {
            log.info("Batch size reached: {}", batchSize);
        }
    }

    private int executeLimited(int limit) {
        var lastNotificationKeyValue = db.currentOrReadOnly(() -> db.keyValues().find(LAST_NOTIFICATION_KEY))
                .orElse(new KeyValue(LAST_NOTIFICATION_KEY, Instant.EPOCH.toString()));

        var lastNotification = Instant.parse(lastNotificationKeyValue.getValue());

        var mutes = db.currentOrReadOnly(() -> db.testMutes().getLastActions(lastNotification, limit));
        var numberOfRecords = mutes.size();

        if (mutes.isEmpty()) {
            return numberOfRecords;
        }
        var lastMute = mutes.get(mutes.size() - 1).getId().getTimestamp();

        // todo temporary redirect all to ci for testing
        mutes = mutes.stream().map(x -> x.toBuilder().service("ci").build()).toList();

        mutes = mutes.stream()
                .filter(x -> x.getService() != null)
                .toList();

        var testsByService = mutes.stream().collect(Collectors.groupingBy(TestMuteEntity::getService));
        log.info(
                "Loaded {} mutes from {} to {}. Affected services: {}",
                mutes.size(), lastNotification, lastMute, testsByService.size()
        );

        var membersByService = testsByService.keySet().stream().collect(
                Collectors.toMap(Function.identity(), this::getServiceMembers)
        );
        for (var serviceGroup : testsByService.entrySet()) {
            var service = serviceGroup.getKey();
            var members = membersByService.get(service);
            var tests = testsByService.get(service);
            log.info("{} tests for {}", tests.size(), service);

            var muted = (int) tests.stream().filter(TestMuteEntity::isMuted).count();
            var byPath = tests.stream().collect(Collectors.groupingBy(TestMuteEntity::getPath));
            var event = CiBadgeEvents.TestMuteChanged.newBuilder()
                    .setNumberOfMutedTests(muted)
                    .setNumberOfUnmutedTests(tests.size() - muted)
                    .setNumberOfPaths(byPath.size())
                    .addAllPaths(
                            byPath.entrySet().stream()
                                    .limit(PATH_LIMIT)
                                    .map(
                                            entry -> convertPath(entry.getKey(), entry.getValue())
                                    ).toList()
                    )
                    .build();


            for (var member : members) {
                badgeEventsSender.sendEvent(
                        CiBadgeEvents.Event.newBuilder()
                                .setTestMuteChanged(
                                        event.toBuilder()
                                                .setOwner(member)
                                                .build()
                                )
                                .build()
                );
            }
        }

        db.currentOrTx(() -> db.keyValues().save(lastNotificationKeyValue.withValue(lastMute.toString())));

        return numberOfRecords;
    }

    private CiBadgeEvents.TestPathMuteInfo convertPath(String path, List<TestMuteEntity> tests) {
        var byTestId = tests.stream().collect(Collectors.groupingBy(x -> x.getId().getTestId().getTestId()));
        return CiBadgeEvents.TestPathMuteInfo.newBuilder()
                .setPath(path)
                .setNumberOfTests(byTestId.size())
                .addAllTests(byTestId.values().stream().limit(TESTS_IN_PATH_LIMIT).map(this::convertTests).toList())
                .build();
    }

    private CiBadgeEvents.TestMuteInfo convertTests(List<TestMuteEntity> tests) {
        var test = tests.get(0);
        var testId = test.getId().getTestId();
        return CiBadgeEvents.TestMuteInfo.newBuilder()
                .setResultType(test.getResultType())
                .setName(test.getName())
                .setSubtestName(test.getSubtestName())
                .setHid(UnsignedLongs.toString(testId.getTestId()))
                .setSuiteHid(UnsignedLongs.toString(testId.getSuiteId()))
                .addAllToolchains(tests.stream().map(this::convertToolchain).toList())
                .build();
    }

    private CiBadgeEvents.ToolchainMuteInfo convertToolchain(TestMuteEntity test) {
        return CiBadgeEvents.ToolchainMuteInfo.newBuilder()
                .setName(test.getId().getTestId().getToolchain())
                .setMuted(test.isMuted())
                .build();
    }

    private List<String> getServiceMembers(String service) {
        // todo abc method to get real service members
        //var members = abcService.getServicesMembers(service).stream().map(x -> x.getPerson().getLogin()).toList();
        var members = List.of("firov");
        log.info("Members of {}: {}", service, members);
        return members;
    }
}
