package ru.yandex.ci.engine.launch.auto;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Value;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.db.model.AutoReleaseSettingsHistory;
import ru.yandex.lang.NonNullApi;

@Value
@AllArgsConstructor
@Nonnull
@NonNullApi
public class AutoReleaseState {

    public static final AutoReleaseState DEFAULT = new AutoReleaseState(false, false, null);

    boolean enabledInConfig;

    boolean enabledForBranchesInConfig;
    @Nullable
    AutoReleaseSettingsHistory latestSettings;

    public AutoReleaseState(boolean enabledInConfig, boolean enabledForBranchesInConfig) {
        this(enabledInConfig, enabledForBranchesInConfig, null);
    }

    public boolean isEditable() {
        return enabledInConfig || enabledForBranchesInConfig;
    }

    public boolean isEnabledInAnyConfig() {
        return isEnabled(ArcBranch.Type.TRUNK) || isEnabled(ArcBranch.Type.RELEASE_BRANCH);
    }

    public boolean isEnabled(ArcBranch.Type type) {
        return switch (type) {
            case TRUNK -> enabledInConfig && isEnabledInLatestSettings();
            case RELEASE_BRANCH -> enabledForBranchesInConfig && isEnabledInLatestSettings();
            case PR, GROUP_BRANCH, USER_BRANCH, UNKNOWN -> throw new RuntimeException("Unsupported branch type "
                    + type);
        };
    }

    private boolean isEnabledInLatestSettings() {
        return latestSettings == null || latestSettings.isEnabled();
    }
}
