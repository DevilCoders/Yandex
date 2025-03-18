package ru.yandex.ci.engine.launch.auto;

import java.util.List;

import javax.annotation.Nullable;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.PostponeLaunch;

/**
 * Handler for specific binary search process
 */
public interface BinarySearchHandler {

    Execution beginExecution();

    interface Execution {
        // Null means we don't know it's settings
        @Nullable
        BinarySearchSettings getBinarySearchSettings(CiProcessId ciProcessId);

        // Empty list to load all launches without virtual type
        List<VirtualCiProcessId.VirtualType> getVirtualTypes();

        ComparisonResult hasNewFailedTests(Launch launchFrom, Launch launchTo);

        void onLaunchStatusChange(PostponeLaunch postponeLaunch);
    }

    enum ComparisonResult {
        NOT_READY,
        NO_NEW_FAILED_TESTS,
        HAS_NEW_FAILED_TESTS,
        LEFT_CANCELED,
        RIGHT_CANCELED,
        BOTH_CANCELED
    }

}
