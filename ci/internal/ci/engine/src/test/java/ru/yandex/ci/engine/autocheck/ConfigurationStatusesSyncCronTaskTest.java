package ru.yandex.ci.engine.autocheck;

import java.time.LocalDateTime;
import java.util.Map;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.te.TestenvDegradationManager.Platform;
import ru.yandex.ci.core.te.TestenvDegradationManager.PlatformStatus;
import ru.yandex.ci.core.te.TestenvDegradationManager.PlatformsStatuses;
import ru.yandex.ci.core.te.TestenvDegradationManager.Status;
import ru.yandex.ci.engine.EngineTestBase;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.when;

class ConfigurationStatusesSyncCronTaskTest extends EngineTestBase {

    private PlatformStatus status(Status enabled) {
        var platformStatus = new PlatformStatus();
        platformStatus.setStatus(enabled);
        platformStatus.setStatusChangeDateTime(LocalDateTime.now(clock));
        return platformStatus;
    }

    @Test
    void test() throws Exception {
        when(testenvDegradationManager.getPrecommitPlatformStatuses())
                .thenReturn(new PlatformsStatuses(Map.of(
                        Platform.SANITIZERS, status(Status.disabled),
                        Platform.GCC_MSVC_MUSL, status(Status.disabled),
                        Platform.IOS_ANDROID_CYGWIN, status(Status.enabled)
                )));

        var task = new ConfigurationStatusesSyncCronTask(null, db, testenvDegradationManager);
        task.executeImpl(null);

        var statuses = db.currentOrReadOnly(() -> db.keyValue().findObject(
                "autocheck", "configurationStatuses", ConfigurationStatuses.class)
        ).orElseThrow();

        assertThat(statuses).isEqualTo(new ConfigurationStatuses(Map.of(
                "sanitizers", false,
                "gcc-msvc-musl", false,
                "ios-android-cygwin", true
        )));
    }
}
