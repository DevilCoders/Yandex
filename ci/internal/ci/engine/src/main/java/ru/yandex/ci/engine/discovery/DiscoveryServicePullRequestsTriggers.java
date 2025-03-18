package ru.yandex.ci.engine.discovery;

import java.util.List;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.core.config.a.model.FilterConfig;

@RequiredArgsConstructor
public class DiscoveryServicePullRequestsTriggers {

    @Nonnull
    private final DiscoveryServiceFilters discoveryServiceFilters;

    public boolean isMatchesAnyFilter(DiscoveryContext context, List<FilterConfig> filters) {
        var suitableFilters = discoveryServiceFilters.getFilter(context).findSuitableFilters(filters);
        return !suitableFilters.isEmpty();
    }
}
