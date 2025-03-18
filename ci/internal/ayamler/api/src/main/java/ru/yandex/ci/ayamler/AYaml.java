package ru.yandex.ci.ayamler;

import java.nio.file.Path;
import java.util.Optional;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Value;
import lombok.With;

import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.AutocheckConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;

@SuppressWarnings("ReferenceEquality")
@Value
public class AYaml {
    @Nonnull
    Path path;
    boolean valid;
    @Nullable
    String errorMessage;

    @Nullable
    String service;         // can be null when ayaml is invalid
    @With
    @Nullable
    StrongMode strongMode;  // null when is not defined in a.yaml or a.yaml is invalid

    @With
    boolean isOwner;  // null when is not defined in a.yaml or a.yaml is invalid

    public static AYaml valid(
            @Nonnull Path path, @Nonnull String service, @Nullable StrongMode strongMode, boolean isOwner
    ) {
        return new AYaml(path, true, null, service, strongMode, isOwner);
    }

    public static AYaml invalid(@Nonnull Path path, @Nullable String errorMessage, boolean isOwner) {
        return new AYaml(path, false, errorMessage, null, null, isOwner);
    }

    @SuppressWarnings("ConstantConditions")
    public static AYaml valid(Path path, AYamlConfig aYamlConfig) {
        StrongMode strongMode = Optional.ofNullable(aYamlConfig)
                .map(AYamlConfig::getCi)
                .map(CiConfig::getAutocheck)
                .map(AutocheckConfig::getStrongMode)
                .map(v -> new StrongMode(v.isEnabled(), v.getAbcScopes()))
                .orElse(null);
        return valid(path, aYamlConfig.getService(), strongMode, false);
    }
}
