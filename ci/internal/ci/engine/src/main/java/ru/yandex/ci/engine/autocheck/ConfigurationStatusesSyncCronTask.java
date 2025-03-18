package ru.yandex.ci.engine.autocheck;

import java.time.Duration;
import java.util.Map;

import javax.annotation.Nullable;

import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.te.TestenvDegradationManager;
import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

public class ConfigurationStatusesSyncCronTask extends CiEngineCronTask {

    private final CiMainDb db;

    private final TestenvDegradationManager testenvDegradationManager;

    public ConfigurationStatusesSyncCronTask(
            @Nullable CuratorFramework curator,
            CiMainDb db,
            TestenvDegradationManager testenvDegradationManager
    ) {
        super(Duration.ofSeconds(30), Duration.ofSeconds(30), curator);
        this.db = db;
        this.testenvDegradationManager = testenvDegradationManager;
    }

    @Override
    protected void executeImpl(ExecutionContext executionContext) throws Exception {
        var configurationIdToStatus = getConfigurationIdToStatus();
        var value = new ConfigurationStatuses(configurationIdToStatus);
        db.currentOrTx(() -> db.keyValue().setValue(
                ConfigurationStatuses.KEY.getNamespace(),
                ConfigurationStatuses.KEY.getKey(),
                value
        ));
    }

    private Map<String, Boolean> getConfigurationIdToStatus() {
        var configurationStatuses = testenvDegradationManager.getPrecommitPlatformStatuses();

        return Map.of(
                "gcc-msvc-musl",
                configurationStatuses.isPlatformEnabled(TestenvDegradationManager.Platform.GCC_MSVC_MUSL),
                "ios-android-cygwin",
                configurationStatuses.isPlatformEnabled(TestenvDegradationManager.Platform.IOS_ANDROID_CYGWIN),
                "sanitizers",
                configurationStatuses.isPlatformEnabled(TestenvDegradationManager.Platform.SANITIZERS)
        );
    }
}
