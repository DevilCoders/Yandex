package ru.yandex.ci.storage.core.db.model.check_iteration;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.util.gson.PrettyJson;
import ru.yandex.ci.ydb.Persisted;
import ru.yandex.inside.yt.kosher.impl.ytree.object.NullSerializationStrategy;
import ru.yandex.inside.yt.kosher.impl.ytree.object.annotation.YTreeObject;

@Persisted
@Value
@Builder
@YTreeObject(nullSerializationStrategy = NullSerializationStrategy.IGNORE_NULL_FIELDS)
@AllArgsConstructor
// Using nullable fields to prevent empty statistics store in db and memory cache.
public class ExtendedStatistics {
    public static final ExtendedStatistics EMPTY = new ExtendedStatistics(
            null, null, null, null, null, null, null
    );

    @Nullable
    ExtendedStageStatistics added;

    @Nullable
    ExtendedStageStatistics deleted;

    @Nullable
    ExtendedStageStatistics flaky;

    @Nullable
    ExtendedStageStatistics muted;

    @Nullable
    ExtendedStageStatistics timeout;

    @Nullable
    ExtendedStageStatistics external;

    @Nullable
    ExtendedStageStatistics internal;

    public ExtendedStageStatistics getAddedOrEmpty() {
        return added == null ? ExtendedStageStatistics.EMPTY : added;
    }

    public ExtendedStageStatistics getDeletedOrEmpty() {
        return deleted == null ? ExtendedStageStatistics.EMPTY : deleted;
    }

    public ExtendedStageStatistics getFlakyOrEmpty() {
        return flaky == null ? ExtendedStageStatistics.EMPTY : flaky;
    }

    public ExtendedStageStatistics getMutedOrEmpty() {
        return muted == null ? ExtendedStageStatistics.EMPTY : muted;
    }

    public ExtendedStageStatistics getTimeoutOrEmpty() {
        return timeout == null ? ExtendedStageStatistics.EMPTY : timeout;
    }

    public ExtendedStageStatistics getExternalOrEmpty() {
        return external == null ? ExtendedStageStatistics.EMPTY : external;
    }

    public ExtendedStageStatistics getInternalOrEmpty() {
        return internal == null ? ExtendedStageStatistics.EMPTY : internal;
    }

    public Mutable toMutable() {
        return new Mutable(
                ExtendedStageStatistics.toMutable(added),
                ExtendedStageStatistics.toMutable(deleted),
                ExtendedStageStatistics.toMutable(flaky),
                ExtendedStageStatistics.toMutable(muted),
                ExtendedStageStatistics.toMutable(timeout),
                ExtendedStageStatistics.toMutable(external),
                ExtendedStageStatistics.toMutable(internal)
        );
    }

    public boolean isImportant() {
        return (added != null && added.getTotal() > 0) ||
                (deleted != null && deleted.getTotal() > 0) ||
                (flaky != null && flaky.getTotal() > 0) ||
                (muted != null && muted.getTotal() > 0) ||
                (timeout != null && timeout.getTotal() > 0) ||
                (external != null && external.getTotal() > 0) ||
                (internal != null && internal.getTotal() > 0);
    }

    @Value
    public static class Mutable {
        ExtendedStageStatistics.Mutable added;
        ExtendedStageStatistics.Mutable deleted;
        ExtendedStageStatistics.Mutable flaky;
        ExtendedStageStatistics.Mutable muted;
        ExtendedStageStatistics.Mutable timeout;
        ExtendedStageStatistics.Mutable external;
        ExtendedStageStatistics.Mutable internal;

        public void add(Mutable statistics, int sign) {
            added.add(statistics.getAdded(), sign);
            deleted.add(statistics.getDeleted(), sign);
            flaky.add(statistics.getFlaky(), sign);
            muted.add(statistics.getMuted(), sign);
            timeout.add(statistics.getTimeout(), sign);
            external.add(statistics.getExternal(), sign);
            internal.add(statistics.getInternal(), sign);
        }

        public ExtendedStatistics toImmutable() {
            var empty = ExtendedStageStatistics.Mutable.EMPTY;
            return new ExtendedStatistics(
                    added.equals(empty) ? null : added.toImmutable(),
                    deleted.equals(empty) ? null : deleted.toImmutable(),
                    flaky.equals(empty) ? null : flaky.toImmutable(),
                    muted.equals(empty) ? null : muted.toImmutable(),
                    timeout.equals(empty) ? null : timeout.toImmutable(),
                    external.equals(empty) ? null : external.toImmutable(),
                    internal.equals(empty) ? null : internal.toImmutable()
            );
        }

        public Mutable normalized() {
            return new Mutable(
                    added.normalized(),
                    deleted.normalized(),
                    flaky.normalized(),
                    muted.normalized(),
                    timeout.normalized(),
                    external.normalized(),
                    internal.normalized()
            );
        }

        public boolean hasNegativeNumbers() {
            return added.hasNegativeNumbers() ||
                    deleted.hasNegativeNumbers() ||
                    flaky.hasNegativeNumbers() ||
                    muted.hasNegativeNumbers() ||
                    timeout.hasNegativeNumbers() ||
                    external.hasNegativeNumbers() ||
                    internal.hasNegativeNumbers();
        }
    }

    @Override
    public String toString() {
        return PrettyJson.format(this);
    }
}
