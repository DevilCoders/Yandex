package ru.yandex.ci.engine.discovery.filter;

import java.util.Map;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import lombok.Value;

import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.ci.core.discovery.DiscoveryType;

@Value
class PathFilters {

    @Nonnull
    Map<DiscoveryType, PathFilterForDiscoveryType> filters;

    private PathFilters(@Nonnull Map<DiscoveryType, PathFilterForDiscoveryType> filters) {
        Preconditions.checkArgument(
                !filters.isEmpty() && !filters.values().stream().allMatch(PathFilterForDiscoveryType::isEmpty),
                "Result filters can not be empty"
        );
        this.filters = filters;
    }

    static PathFilters of(DiscoveryType contextDiscoveryType, FilterConfig filterConfig) {
        if (contextDiscoveryType == DiscoveryType.STORAGE) {
            throw new DiscoveryTypeNotSupportedException(DiscoveryType.STORAGE);
        }

        var requiredDiscoveryType = switch (filterConfig.getDiscovery()) {
            case DIR, DEFAULT -> DiscoveryType.DIR;
            case GRAPH -> DiscoveryType.GRAPH;
            case ANY -> mapValueAnyToRequiredDiscoveryType(filterConfig, contextDiscoveryType);
            case PCI_DSS -> DiscoveryType.PCI_DSS;
        };

        return new PathFilters(
                Map.of(requiredDiscoveryType, PathFilterForDiscoveryType.of(filterConfig))
        );
    }

    private static DiscoveryType mapValueAnyToRequiredDiscoveryType(
            FilterConfig filterConfig,
            DiscoveryType contextDiscoveryType
    ) {
        Preconditions.checkArgument(filterConfig.getDiscovery() == FilterConfig.Discovery.ANY);
        return switch (contextDiscoveryType) {
            case DIR -> DiscoveryType.DIR;
            case GRAPH -> DiscoveryType.GRAPH;
            case PCI_DSS -> DiscoveryType.PCI_DSS;
            default -> throw new DiscoveryTypeNotSupportedException(contextDiscoveryType);
        };
    }

    PathFilterForDiscoveryType getDir() {
        return filters.getOrDefault(DiscoveryType.DIR, PathFilterForDiscoveryType.empty());
    }

    PathFilterForDiscoveryType getGraph() {
        return filters.getOrDefault(DiscoveryType.GRAPH, PathFilterForDiscoveryType.empty());
    }

    PathFilterForDiscoveryType getPciDss() {
        return filters.getOrDefault(DiscoveryType.PCI_DSS, PathFilterForDiscoveryType.empty());
    }

    boolean isAnyOtherDiscoveryRequired(DiscoveryType except) {
        for (var discoveryType : DiscoveryType.values()) {
            if (discoveryType == except) {
                continue;
            }
            var filter = filters.getOrDefault(discoveryType, PathFilterForDiscoveryType.empty());
            if (!filter.isEmpty()) {
                return true;
            }
        }
        return false;
    }

}
