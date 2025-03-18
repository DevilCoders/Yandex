package ru.yandex.ci.core.config.a.model;

import java.util.List;
import java.util.Map;
import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;

@Data
@AllArgsConstructor
@NoArgsConstructor
public class LargeAutostartConfig {
    private static final String TARGET_FIELD = "target";
    private static final String TOOLCHAINS_FIELD = "toolchains";

    @Nonnull
    private String target;

    @Nullable
    private List<String> toolchains;

    public LargeAutostartConfig(@Nonnull String target) {
        this(target, null);
    }

    public List<String> getToolchains() {
        return Objects.requireNonNullElse(toolchains, List.of());
    }

    @SuppressWarnings("unchecked")
    public static LargeAutostartConfig fromMap(Map<?, ?> largeAutostartMap) {
        List<String> toolchains = null;
        if (largeAutostartMap.get(TOOLCHAINS_FIELD) instanceof List) {
            toolchains = (List<String>) largeAutostartMap.get(TOOLCHAINS_FIELD);
        } else if (largeAutostartMap.get(TOOLCHAINS_FIELD) instanceof String) {
            toolchains = List.of(largeAutostartMap.get(TOOLCHAINS_FIELD).toString());
        }

        return new LargeAutostartConfig(
                largeAutostartMap.get(TARGET_FIELD).toString(),
                toolchains
        );
    }
}
