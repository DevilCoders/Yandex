package ru.yandex.ci.storage.core.db.model.test_diff;

import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StageStatistics;
import ru.yandex.ci.ydb.Persisted;
import ru.yandex.inside.yt.kosher.impl.ytree.object.annotation.YTreeObject;

/*
  Statistics hierarchy of a test from toolchain T. The test affects all nested aggregates.

               Test (T)
           /              \
    Test (@all)         Suite(T)
        |                  |
    Suite (@all)         Chunk(T)
        |
     Chunk (@all)
*/

@Persisted
@Value
@YTreeObject
public class TestDiffStatistics {
    public static final TestDiffStatistics EMPTY = new TestDiffStatistics(
            StatisticsGroup.EMPTY, StatisticsGroup.EMPTY
    );

    StatisticsGroup self;
    StatisticsGroup children;

    public Mutable toMutable() {
        return new Mutable(self.toMutable(), children.toMutable());
    }

    @Value
    public static class Mutable {
        StatisticsGroupMutable self;
        StatisticsGroupMutable children;

        public TestDiffStatistics toImmutable() {
            return new TestDiffStatistics(self.toImmutable(), children.toImmutable());
        }
    }

    @Persisted
    @Value
    @YTreeObject
    public static class StatisticsGroup {
        public static final StatisticsGroup EMPTY = new StatisticsGroup(
                StageStatistics.EMPTY, ExtendedStatistics.EMPTY
        );

        StageStatistics stage;
        ExtendedStatistics extended;

        public StatisticsGroup(StageStatistics stage, @Nullable ExtendedStatistics extended) {
            this.stage = stage;
            // this can be null when all nested fields are null.
            this.extended = extended == null ? ExtendedStatistics.EMPTY : extended;
        }

        public StatisticsGroupMutable toMutable() {
            return new StatisticsGroupMutable(stage.toMutable(), extended.toMutable());
        }

        public boolean isEmpty() {
            return this.equals(EMPTY);
        }

        public boolean isImportant() {
            return stage.isImportant() || extended.isImportant();
        }
    }

    @Value
    public static class StatisticsGroupMutable {
        StageStatistics.Mutable stage;
        ExtendedStatistics.Mutable extended;

        public StatisticsGroup toImmutable() {
            return new StatisticsGroup(stage.toImmutable(), extended.toImmutable());
        }

        public void add(StatisticsGroupMutable group, int sign) {
            this.stage.add(group.getStage(), sign);
            this.extended.add(group.getExtended(), sign);
        }

        public boolean hasNegativeNumbers() {
            return this.stage.hasNegativeNumbers() || this.extended.hasNegativeNumbers();
        }

        public StatisticsGroupMutable normalized() {
            return new StatisticsGroupMutable(
                    stage.normalized(),
                    extended.normalized()
            );
        }
    }
}
