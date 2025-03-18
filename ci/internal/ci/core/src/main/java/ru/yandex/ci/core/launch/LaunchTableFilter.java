package ru.yandex.ci.core.launch;

import java.util.List;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.GetFlowLaunchesRequest;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.OrderByDirection;
import ru.yandex.lang.NonNullApi;

@Value
@NonNullApi
@Builder(toBuilder = true)
public class LaunchTableFilter {

    private static final LaunchTableFilter EMPTY = LaunchTableFilter.builder().build();

    @Nullable
    Boolean pinned;

    @Nonnull
    @Singular
    List<String> tags;

    @Nullable
    String branch;

    @Nonnull
    @Singular
    List<LaunchState.Status> statuses;

    @Nonnull
    GetFlowLaunchesRequest.OrderBy sortBy;

    @Nonnull
    OrderByDirection sortDirection;

    public static LaunchTableFilter empty() {
        return EMPTY;
    }

    public static class Builder {
        {
            sortBy = GetFlowLaunchesRequest.OrderBy.NUMBER;
            sortDirection = OrderByDirection.DESC;
        }
    }
}
