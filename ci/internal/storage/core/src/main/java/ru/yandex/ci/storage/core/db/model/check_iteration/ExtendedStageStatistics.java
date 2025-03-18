package ru.yandex.ci.storage.core.db.model.check_iteration;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;
import ru.yandex.inside.yt.kosher.impl.ytree.object.annotation.YTreeObject;

@SuppressWarnings("AmbiguousMethodReference")
@Persisted
@Value
@Builder
@YTreeObject
@AllArgsConstructor
public class ExtendedStageStatistics {
    public static final ExtendedStageStatistics EMPTY = new ExtendedStageStatistics(0, 0, 0);

    int total;
    int passedAdded;
    int failedAdded;

    public static Mutable toMutable(@Nullable ExtendedStageStatistics value) {
        return value == null ? new Mutable(0, 0, 0) : value.toMutable();
    }

    public Mutable toMutable() {
        return new Mutable(
                total,
                passedAdded,
                failedAdded
        );
    }

    @Data
    @lombok.Builder
    @AllArgsConstructor
    public static class Mutable {
        public static final Mutable EMPTY = new Mutable(0, 0, 0);

        int total;
        int passedAdded;
        int failedAdded;

        public void add(@Nullable Mutable statistics, int sign) {
            if (statistics == null) {
                return;
            }

            if (sign > 0) {
                this.total += statistics.total;
                this.passedAdded += statistics.passedAdded;
                this.failedAdded += statistics.failedAdded;
            } else {
                this.total -= statistics.total;
                this.passedAdded -= statistics.passedAdded;
                this.failedAdded -= statistics.failedAdded;
            }
        }

        public ExtendedStageStatistics toImmutable() {
            return new ExtendedStageStatistics(
                    total,
                    passedAdded,
                    failedAdded
            );
        }

        public Mutable normalized() {
            return new Mutable(
                    total > 0 ? 1 : 0,
                    passedAdded > 0 ? 1 : 0,
                    failedAdded > 0 ? 1 : 0
            );

        }

        public boolean hasNegativeNumbers() {
            return total < 0 || passedAdded < 0 || failedAdded < 0;
        }
    }
}
