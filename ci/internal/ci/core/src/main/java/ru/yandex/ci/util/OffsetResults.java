package ru.yandex.ci.util;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Function;
import java.util.function.LongSupplier;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;

import ru.yandex.ci.common.ydb.TransactionSupportDefault;
import ru.yandex.ci.common.ydb.YdbUtils;
import ru.yandex.lang.NonNullApi;

@NonNullApi
@AllArgsConstructor(staticName = "of", access = AccessLevel.PRIVATE)
public class OffsetResults<T> {
    private final List<T> items;

    @Nullable
    private final Long total;

    private final boolean hasMore;

    private static <T> OffsetResults<T> doRequest(
            int limit,
            Request<T> itemsSupplier,
            @Nullable LongSupplier totalSupplier
    ) {
        Preconditions.checkArgument(limit <= YdbUtils.RESULT_ROW_LIMIT,
                "limit cannot be greater than %s, got %s", YdbUtils.RESULT_ROW_LIMIT, limit);

        int requestLimit = (limit > 0 && limit < YdbUtils.RESULT_ROW_LIMIT) // limit = 0 - значит все результаты
                ? limit + 1
                : limit;

        List<T> items = itemsSupplier.request(requestLimit);

        boolean hasMore = limit > 0 && (items.size() > limit || limit == YdbUtils.RESULT_ROW_LIMIT);

        if (hasMore) {
            items = new ArrayList<>(items.subList(0, limit));
        }

        Long total = totalSupplier != null
                // Нет смысла делать запрос - все записи уже найдены
                ? ((limit <= 0 || hasMore) ? totalSupplier.getAsLong() : items.size())
                : null;

        return OffsetResults.of(items, total, hasMore);
    }

    public <TT> OffsetResults<TT> mapItems(Function<T, TT> mapper) {
        return OffsetResults.of(
                items.stream().map(mapper).toList(),
                total,
                hasMore
        );
    }

    @FunctionalInterface
    public interface Request<T> {
        List<T> request(int limit);
    }

    public static <T> OffsetResults<T> empty() {
        return new OffsetResults<>(List.of(), 0L, false);
    }

    public static Builder builder() {
        return new Builder();
    }

    public static class Builder {
        int limit;
        LongSupplier totalSupplier;
        @Nullable
        TransactionSupportDefault db;
        boolean readOnlyTransaction;

        public <T> SupplierStep<T> withItems(int limit, Request<T> request) {
            this.limit = limit;
            return new SupplierStep<>(request);
        }

        public TransactionStep withTransactionReadWrite(TransactionSupportDefault db) {
            return withTransaction(db, false);
        }

        public TransactionStep withTransactionReadOnly(TransactionSupportDefault db) {
            return withTransaction(db, true);
        }

        private TransactionStep withTransaction(TransactionSupportDefault db, boolean readOnly) {
            this.db = db;
            this.readOnlyTransaction = readOnly;
            return new TransactionStep();
        }

        public class TransactionStep {
            private TransactionStep() {
            }

            public <T> SupplierStep<T> withItems(int limit, Request<T> request) {
                return Builder.this.withItems(limit, request);
            }
        }

        public class SupplierStep<T> {
            private final Request<T> request;

            private SupplierStep(Request<T> request) {
                this.request = request;
            }

            public OffsetResults<T> fetch() {
                if (db != null) {
                    if (readOnlyTransaction) {
                        return db.currentOrReadOnly(() -> doRequest(limit, request, totalSupplier));
                    } else {
                        return db.currentOrTx(() -> doRequest(limit, request, totalSupplier));
                    }
                }
                return doRequest(limit, request, totalSupplier);
            }

            public TotalStep withTotal(LongSupplier totalSupplier) {
                Builder.this.totalSupplier = totalSupplier;
                return new TotalStep();
            }

            public class TotalStep {
                private TotalStep() {
                }

                public OffsetResults<T> fetch() {
                    return SupplierStep.this.fetch();
                }
            }
        }
    }

    public List<T> items() {
        return items;
    }

    public boolean hasMore() {
        return hasMore;
    }

    @Nullable
    public Long getTotal() {
        return total;
    }
}
