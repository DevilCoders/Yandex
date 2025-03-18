package ru.yandex.ci.engine.discovery.tier0;

import java.nio.file.Path;
import java.util.Optional;

import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.a.ConfigChangeType;
import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.discovery.DiscoveryContext;

final class GraphDiscoveryHelper {
    private GraphDiscoveryHelper() {
    }

    static String platformToSandboxPlatformParam(GraphDiscoveryTask.Platform source) {
        return switch (source) {
            case LINUX -> "linux";
            case MANDATORY -> "mandatory";
            case SANITIZERS -> "sanitizers";
            case GCC_MSVC_MUSL -> "gcc-msvc-musl";
            case IOS_ANDROID_CYGWIN -> "ios-android-cygwin";
        };
    }

    static GraphDiscoveryTask.Platform sandboxPlatformParamToPlatform(String source) {
        return switch (source) {
            case "linux" -> GraphDiscoveryTask.Platform.LINUX;
            case "mandatory" -> GraphDiscoveryTask.Platform.MANDATORY;
            case "sanitizers" -> GraphDiscoveryTask.Platform.SANITIZERS;
            case "gcc-msvc-musl" -> GraphDiscoveryTask.Platform.GCC_MSVC_MUSL;
            case "ios-android-cygwin" -> GraphDiscoveryTask.Platform.IOS_ANDROID_CYGWIN;
            default -> throw new IllegalStateException("Unexpected value: " + source);
        };
    }

    static String platformToSandboxSemaphoreName(
            GraphDiscoveryTask.Platform source,
            boolean useDistbuildTestingCluster
    ) {
        var cluster = useDistbuildTestingCluster ? "testing" : "production";
        return switch (source) {
            case LINUX, MANDATORY -> "CI_GRAPH_DISCOVERY_LINUX_AND_MANDATORY.distbuild-" + cluster;
            case SANITIZERS -> "CI_GRAPH_DISCOVERY_SANITIZERS.distbuild-" + cluster;
            case GCC_MSVC_MUSL -> "CI_GRAPH_DISCOVERY_GCC_MSVC_MUSL.distbuild-" + cluster;
            case IOS_ANDROID_CYGWIN -> "CI_GRAPH_DISCOVERY_IOS_ANDROID_CYGWIN.distbuild-" + cluster;
        };
    }

    static LruCache<Path, Optional<ConfigBundle>> createConfigBundleCache(
            ConfigurationService configurationService,
            ArcCommit commit,
            OrderedArcRevision revision,
            int configBundleCacheSize
    ) {
        return new LruCache<>(
                configBundleCacheSize,
                aYamlPath -> configurationService
                        .getOrCreateBranchConfig(aYamlPath, commit, revision)
                        .map(configurationService::getLastActualConfig)
        );
    }

    static DiscoveryContext.Builder discoveryContextBuilder(boolean shouldContainAbsPathFilter) {
        return DiscoveryContext.builder()
                .configChange(ConfigChangeType.NONE)
                .discoveryType(DiscoveryType.GRAPH)
                .filterPredicate(
                        filter -> (!shouldContainAbsPathFilter || !filter.getAbsPaths().isEmpty()) &&
                                (filter.getDiscovery() == FilterConfig.Discovery.ANY
                                        || filter.getDiscovery() == FilterConfig.Discovery.GRAPH)
                );

    }

}
