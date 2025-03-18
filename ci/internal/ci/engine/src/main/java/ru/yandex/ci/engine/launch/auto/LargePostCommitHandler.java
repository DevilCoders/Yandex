package ru.yandex.ci.engine.launch.auto;


import java.time.Duration;
import java.util.List;
import java.util.Set;

import javax.annotation.Nonnull;

import com.google.protobuf.InvalidProtocolBufferException;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.PostponeLaunch;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.commune.bazinga.BazingaTaskManager;

// Large tests and Native builds binary search
@Slf4j
@RequiredArgsConstructor
public class LargePostCommitHandler implements BinarySearchHandler, BinarySearchHandler.Execution {

    @Nonnull
    private final StorageApiClient storageApiClient;
    @Nonnull
    private final BazingaTaskManager bazingaTaskManager;
    @Nonnull
    private final Duration delayBetweenLaunches;
    @Nonnull
    private final Duration delayBetweenSlowLaunches;
    @Nonnull
    private final Duration closeLaunchesOlderThan;
    private final int maxActiveLaunches;

    @Override
    public Execution beginExecution() {
        return this; // No caching required
    }

    @Override
    public BinarySearchSettings getBinarySearchSettings(CiProcessId ciProcessId) {
        return BinarySearchSettings.builder()
                .minIntervalDuration(getDuration(ciProcessId))
                .minIntervalSize(0)
                .closeLaunchesOlderThan(closeLaunchesOlderThan)
                .maxActiveLaunches(maxActiveLaunches)
                .discoveryTypes(Set.of(DiscoveryType.STORAGE))
                .build();
    }

    @Override
    public List<VirtualType> getVirtualTypes() {
        // VIRTUAL_NATIVE_BUILD actual does nothing (we have no option to run Native builds in post-commits yet)
        return List.of(VirtualType.VIRTUAL_LARGE_TEST, VirtualType.VIRTUAL_NATIVE_BUILD);
    }

    @Override
    public ComparisonResult hasNewFailedTests(Launch launchFrom, Launch launchTo) {
        try {
            var request = StorageApi.CompareLargeTasksRequest.newBuilder()
                    .setLeft(toLargeTaskId(launchFrom))
                    .setRight(toLargeTaskId(launchTo))
                    .addAllFilterDiffTypes(List.of(
                            Common.TestDiffType.TDT_FAILED,
                            Common.TestDiffType.TDT_FAILED_BROKEN,
                            Common.TestDiffType.TDT_FAILED_NEW
                    ))
                    .build();
            var response = storageApiClient.compareLargeTasks(request);
            log.info("Total diffs in response: {}", response.getDiffsCount());

            if (!response.getCompareReady()) {
                log.info("Comparison is not ready");
                return ComparisonResult.NOT_READY;
            }
            if (response.getCanceledLeft() && response.getCanceledRight()) {
                return ComparisonResult.BOTH_CANCELED;
            } else if (response.getCanceledLeft()) {
                return ComparisonResult.LEFT_CANCELED;
            } else if (response.getCanceledRight()) {
                return ComparisonResult.RIGHT_CANCELED;
            }

            for (var diff : response.getDiffsList()) {
                log.info("Has new failed tests starting from {}", diff);
                return ComparisonResult.HAS_NEW_FAILED_TESTS;
            }

            return ComparisonResult.NO_NEW_FAILED_TESTS;
        } catch (Exception e) {
            log.error("Unable to check if we have new failed tests", e);
            return ComparisonResult.NOT_READY;
        }
    }

    @Override
    public void onLaunchStatusChange(PostponeLaunch postponeLaunch) {
        if (LargePostCommitWriterTask.acceptPostpone(postponeLaunch.getStatus())) {
            log.info("Scheduling Storage task integration");
            bazingaTaskManager.schedule(new LargePostCommitWriterTask(postponeLaunch.getId()));
        } else {
            log.info("No Storage integration is required");
        }
    }

    static StorageApi.LargeTaskId toLargeTaskId(Launch launch) throws InvalidProtocolBufferException {
        return LargePostCommitWriterTask.toLargeTaskId(launch);
    }

    private Duration getDuration(CiProcessId ciProcessId) {
        // All fuzzing, msan and asan runs considered as 'slow'; just like in TE
        var id = ciProcessId.getSubId();
        var isSlow = id.contains("-fuzzing@") || id.contains("-msan@") || id.contains("-asan@");
        var duration = isSlow
                ? delayBetweenSlowLaunches
                : delayBetweenLaunches;
        log.info("Consider {} as {}, use duration {}",
                ciProcessId, isSlow ? "SLOW" : "NORMAL", duration);
        return duration;
    }

}
