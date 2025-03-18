package ru.yandex.ci.flow.engine.definition.stage;

import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.EqualsAndHashCode;
import lombok.ToString;
import lombok.Value;

import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchState;

/**
 * Описание синхронизированной стадии. К стадиям можно привязать джобы флоу с помощью
 * {@link JobBuilder#beginStage(Stage)}, в таком случае будет гарантировано, что в определённый момент времени
 * только один флоу исполняет джобы этой стадии.
 */
@Value
@AllArgsConstructor
public class Stage implements Comparable<Stage>, StageRef {

    @Nonnull
    @ToString.Exclude
    StageGroup stageGroup;

    @Nonnull
    String id;

    @Nullable
    String title;

    @EqualsAndHashCode.Exclude
    boolean canBeInterrupted;

    @Nonnull
    Set<LaunchState> displacementOptions;

    boolean rollback;

    @Override
    public String getId() {
        return id;
    }

    @Override
    public int compareTo(@Nonnull Stage other) {
        int thisIndex = stageGroup.getStages().indexOf(this);
        Preconditions.checkState(
            thisIndex >= 0,
            "Incorrect Stage object: stage %s is not in stage group %s",
            this, stageGroup
        );
        int otherIndex = stageGroup.getStages().indexOf(other);
        Preconditions.checkArgument(
            otherIndex >= 0,
            "Stage %s is not in stage group %s", other, stageGroup
        );
        return thisIndex - otherIndex;
    }
}
