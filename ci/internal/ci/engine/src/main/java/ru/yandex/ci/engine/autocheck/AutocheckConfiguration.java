package ru.yandex.ci.engine.autocheck;


import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.Value;

import ru.yandex.ci.engine.autocheck.config.AutocheckConfigurationConfig;

@Value
public class AutocheckConfiguration {
    String id;
    @Nullable
    AutocheckConfigurationConfig leftConfig;
    @Nullable
    AutocheckConfigurationConfig rightConfig;

    public boolean hasLeftTask() {
        return leftConfig != null;
    }

    public boolean hasRightTask() {
        return rightConfig != null;
    }

    public AutocheckConfigurationConfig getLeftConfig() {
        Preconditions.checkState(hasLeftTask());
        return leftConfig;
    }

    public AutocheckConfigurationConfig getRightConfig() {
        Preconditions.checkState(hasRightTask());
        return rightConfig;
    }
}
