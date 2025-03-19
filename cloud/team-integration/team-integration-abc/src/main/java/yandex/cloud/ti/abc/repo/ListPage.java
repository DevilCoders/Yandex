package yandex.cloud.ti.abc.repo;

import java.util.List;
import java.util.function.Function;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

public record ListPage<T>(
        @NotNull List<? extends T> items,
        @Nullable String nextPageToken
) {

    public <R> ListPage<R> map(@NotNull Function<? super T, ? extends R> mapper) {
        return new ListPage<>(
                items.stream().map(mapper).toList(),
                nextPageToken
        );
    }

}
