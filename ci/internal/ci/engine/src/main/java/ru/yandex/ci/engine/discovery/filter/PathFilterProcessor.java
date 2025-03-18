package ru.yandex.ci.engine.discovery.filter;

import java.util.List;

import com.google.common.annotations.VisibleForTesting;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.engine.discovery.DiscoveryContext;
import ru.yandex.ci.util.GlobMatchers;

@Slf4j
public final class PathFilterProcessor {

    private PathFilterProcessor() {
    }

    public static boolean hasCorrectPaths(DiscoveryContext context, FilterConfig filterConfig) {
        // No filters must be used when collecting triggers for release branches
        if (context.isReleaseBranch()) {
            return true;
        }

        var filters = PathFilters.of(context.getDiscoveryType(), filterConfig);

        switch (context.getDiscoveryType()) {
            case DIR -> {
                var success = hasCorrectPaths(context, filters.getDir());
                /* When one filter requires, for example, dir and graph discovery,
                   we can say something only when graph discovery is finished */
                return success && !filters.isAnyOtherDiscoveryRequired(DiscoveryType.DIR);
            }
            case GRAPH -> {
                // TODO: support case `if (!filters.isOnlyGraphRequired())`
                return hasCorrectPaths(context, filters.getGraph());
            }
            case PCI_DSS -> {
                // TODO: support case `if (!filters.isOnlyPciDssRequired())`
                return hasCorrectPaths(context, filters.getPciDss());
            }
            default -> throw new DiscoveryTypeNotSupportedException(context.getDiscoveryType());
        }
    }

    private static boolean hasCorrectPaths(DiscoveryContext context, PathFilterForDiscoveryType filter) {
        if (!hasCorrectSubPath(context, filter.getSubPaths(), filter.getNotSubPaths())) {
            log.info("Filter {} filtered commit by sub-paths", filter);
            return false;
        }

        if (!hasCorrectAbsPath(context, filter.getAbsPaths(), filter.getNotAbsPaths())) {
            log.info("Filter {} filtered commit by abs-paths", filter);
            return false;
        }
        return true;
    }

    private static boolean hasCorrectSubPath(
            DiscoveryContext context,
            List<String> subPaths,
            List<String> notSubPaths
    ) {
        if (subPaths.isEmpty() && notSubPaths.isEmpty()) {
            return true;
        }
        var configDir = AYamlService.pathToDir(context.getConfigPath());
        if (!configDir.isEmpty()) {
            configDir += "/";
        }

        var dir = GlobMatchers.escapeMetaChars(configDir);

        return GlobMatchers.pathsMatchGlobs(
                context.getAffectedPaths(),
                addDirectoryToBeginOfPaths(dir, subPaths),
                addDirectoryToBeginOfPaths(dir, notSubPaths)
        );
    }

    private static boolean hasCorrectAbsPath(
            DiscoveryContext context,
            List<String> absPaths,
            List<String> notAbsPaths
    ) {
        // keep evaluation of `getAffectedPaths()` lazy
        if (absPaths.isEmpty() && notAbsPaths.isEmpty()) {
            return true;
        }
        return switch (context.getDiscoveryType()) {
            case DIR, GRAPH -> GlobMatchers.pathsMatchGlobs(
                    context.getAffectedPaths(), absPaths, notAbsPaths);
            case STORAGE, PCI_DSS -> false;
        };
    }

    @VisibleForTesting
    static List<String> addDirectoryToBeginOfPaths(String dir, List<String> subPaths) {
        return subPaths.stream().map(path -> dir + path).toList();
    }

}
