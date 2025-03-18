package ru.yandex.ci.storage.core.db.model.check_iteration;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.ydb.Persisted;
import ru.yandex.inside.yt.kosher.impl.ytree.object.annotation.YTreeObject;

@SuppressWarnings("AmbiguousMethodReference")
@Persisted
@Value
@YTreeObject
@Builder
@AllArgsConstructor
public class StageStatistics {
    public static final StageStatistics EMPTY = new StageStatistics(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    int total;

    int passed;
    int passedAdded;

    int failed;
    int failedAdded;
    int failedInStrongMode;

    int failedByDeps;
    int failedByDepsAdded;
    int passedByDepsAdded;

    int skipped;
    int skippedAdded;

    public static Mutable toMutable(@Nullable StageStatistics value) {
        return value == null ? new Mutable(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) : value.toMutable();
    }

    public Mutable toMutable() {
        return new Mutable(
                total,

                passed,
                passedAdded,

                failed,
                failedAdded,
                failedInStrongMode,

                failedByDeps,
                failedByDepsAdded,
                passedByDepsAdded,

                skipped,
                skippedAdded
        );
    }

    public boolean isImportant() {
        return passedAdded > 0 ||
                failedAdded > 0 ||
                failedByDeps > 0 ||
                failedByDepsAdded > 0 ||
                failedInStrongMode > 0 ||
                passedByDepsAdded > 0 ||
                skippedAdded > 0;
    }

    public Common.CheckStatus toCompletedStatus() {
        return getFailedAdded() > 0 || getFailedByDepsAdded() > 0 || getFailedInStrongMode() > 0
                ? Common.CheckStatus.COMPLETED_FAILED
                : Common.CheckStatus.COMPLETED_SUCCESS;
    }

    @Data
    @lombok.Builder
    @AllArgsConstructor
    public static class Mutable {
        public static final Mutable EMPTY = new Mutable(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

        int total;

        int passed;
        int passedAdded;

        int failed;
        int failedAdded;
        int failedInStrongMode;

        int failedByDeps;
        int failedByDepsAdded;
        int passedByDepsAdded;

        int skipped;
        int skippedAdded;

        public void changePassed(int delta) {
            this.passed += delta;
        }

        public void changePassedAdded(int delta) {
            this.passedAdded += delta;
        }

        public void changeFailed(int delta) {
            this.failed += delta;
        }

        public void changeFailedAdded(int delta) {
            this.failedAdded += delta;
        }

        public void changeFailedInStrongMode(int delta) {
            this.failedInStrongMode += delta;
        }

        public void changeFailedByDeps(int delta) {
            this.failedByDeps += delta;
        }

        public void changeFailedByDepsAdded(int delta) {
            this.failedByDepsAdded += delta;
        }

        public void changePassedByDepsAdded(int delta) {
            this.passedByDepsAdded += delta;
        }

        public void changeSkipped(int delta) {
            this.skipped += delta;
        }

        public void changeSkippedAdded(int delta) {
            this.skippedAdded += delta;
        }

        public void add(@Nullable Mutable statistics, int sign) {
            if (statistics == null) {
                return;
            }

            if (sign > 0) {
                this.total += statistics.total;

                this.passed += statistics.passed;
                this.passedAdded += statistics.passedAdded;

                this.failed += statistics.failed;
                this.failedAdded += statistics.failedAdded;
                this.failedInStrongMode += statistics.failedInStrongMode;

                this.failedByDeps += statistics.failedByDeps;
                this.failedByDepsAdded += statistics.failedByDepsAdded;
                this.passedByDepsAdded += statistics.passedByDepsAdded;

                this.skipped += statistics.skipped;
                this.skippedAdded += statistics.skippedAdded;
            } else {
                this.total -= statistics.total;

                this.passed -= statistics.passed;
                this.passedAdded -= statistics.passedAdded;

                this.failed -= statistics.failed;
                this.failedAdded -= statistics.failedAdded;
                this.failedInStrongMode -= statistics.failedInStrongMode;

                this.failedByDeps -= statistics.failedByDeps;
                this.failedByDepsAdded -= statistics.failedByDepsAdded;
                this.passedByDepsAdded -= statistics.passedByDepsAdded;

                this.skipped -= statistics.skipped;
                this.skippedAdded -= statistics.skippedAdded;
            }
        }

        public StageStatistics toImmutable() {
            return new StageStatistics(
                    total,

                    passed,
                    passedAdded,

                    failed,
                    failedAdded,
                    failedInStrongMode,

                    failedByDeps,
                    failedByDepsAdded,
                    passedByDepsAdded,

                    skipped,
                    skippedAdded
            );
        }

        public Mutable normalized() {
            return new Mutable(
                    total > 0 ? 1 : 0,
                    passed > 0 ? 1 : 0,
                    passedAdded > 0 ? 1 : 0,
                    failed > 0 ? 1 : 0,
                    failedAdded > 0 ? 1 : 0,
                    failedInStrongMode > 0 ? 1 : 0,
                    failedByDeps > 0 ? 1 : 0,
                    failedByDepsAdded > 0 ? 1 : 0,
                    passedByDepsAdded > 0 ? 1 : 0,
                    skipped > 0 ? 1 : 0,
                    skippedAdded > 0 ? 1 : 0
            );
        }

        public boolean hasNegativeNumbers() {
            return
                    total < 0 ||
                            passed < 0 ||
                            passedAdded < 0 ||
                            failed < 0 ||
                            failedAdded < 0 ||
                            failedInStrongMode < 0 ||
                            failedByDeps < 0 ||
                            failedByDepsAdded < 0 ||
                            passedByDepsAdded < 0 ||
                            skipped < 0 ||
                            skippedAdded < 0;
        }
    }
}
