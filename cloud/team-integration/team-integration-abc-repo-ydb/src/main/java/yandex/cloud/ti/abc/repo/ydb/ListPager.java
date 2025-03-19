package yandex.cloud.ti.abc.repo.ydb;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import yandex.cloud.binding.expression.OrderExpression;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.db.list.Expressions;
import yandex.cloud.repository.db.list.ListRequest;
import yandex.cloud.repository.db.list.ListResult;
import yandex.cloud.repository.db.list.token.LastKeyPageToken;
import yandex.cloud.repository.db.list.token.PageToken;
import yandex.cloud.ti.abc.repo.ListPage;

class ListPager<T extends Entity<T>> {

    private final @NotNull Class<T> entityType;
    private final @NotNull OrderExpression<T> orderBy;
    private final @NotNull PageToken pageBy;


    ListPager(@NotNull Class<T> entityType) {
        this.entityType = entityType;
        orderBy = Expressions.defaultOrder(entityType);
        pageBy = LastKeyPageToken.forOrdering(orderBy);
    }


    private @NotNull ListRequest.Builder<T> createListRequestBuilder(
            long pageSize,
            @Nullable String pageToken
    ) {
        ListRequest.Builder<T> builder = ListRequest.builder(entityType);
        builder.orderBy(orderBy);
        builder.pageSize(pageSize);
        if (pageToken != null && !pageToken.isEmpty()) {
            builder.pageToken(pageBy).decode(pageToken);
        }
        return builder;
    }

    @NotNull ListRequest<T> createListRequest(
            long pageSize,
            @Nullable String pageToken
    ) {
        return createListRequestBuilder(pageSize, pageToken)
                .build();
    }

    @NotNull ListPage<T> processListResult(
            @NotNull ListResult<T> result
    ) {
        return new ListPage<>(
                result.getEntries(),
                pageBy.encode(result)
        );
    }

}
