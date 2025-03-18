package ru.yandex.ci.engine.discovery.filter;

import java.util.List;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.core.config.a.model.FilterConfig;

@Value(staticConstructor = "of")
class PathFilterForDiscoveryType {

    private static final PathFilterForDiscoveryType EMPTY =
            PathFilterForDiscoveryType.of(
                    List.of(), List.of(), List.of(), List.of()
            );

    @Nonnull
    List<String> subPaths;
    @Nonnull
    List<String> absPaths;
    @Nonnull
    List<String> notSubPaths;
    @Nonnull
    List<String> notAbsPaths;

    static PathFilterForDiscoveryType empty() {
        return EMPTY;
    }

    static PathFilterForDiscoveryType of(FilterConfig filter) {
        var allPathFiltersEmpty = filter.getSubPaths().isEmpty()
                && filter.getAbsPaths().isEmpty()
                && filter.getNotSubPaths().isEmpty()
                && filter.getNotAbsPaths().isEmpty();

        return PathFilterForDiscoveryType.of(
                // TODO: CI-2844, don't do that in future for FilterConfig.Discovery.DEFAULT
                allPathFiltersEmpty ? List.of("**") : filter.getSubPaths(),
                filter.getAbsPaths(),
                filter.getNotSubPaths(),
                filter.getNotAbsPaths()
        );
    }

    boolean isEmpty() {
        return subPaths.isEmpty() && absPaths.isEmpty() && notSubPaths.isEmpty() && notAbsPaths.isEmpty();
    }

}
