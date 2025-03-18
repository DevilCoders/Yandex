package ru.yandex.ci.flow.engine.runtime.state.model;

import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.flow.engine.definition.stage.Stage;
import ru.yandex.ci.flow.engine.definition.stage.StageRef;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder
@JsonDeserialize(builder = StoredStage.Builder.class)
public class StoredStage implements StageRef {

    @Nonnull
    String id;

    @Nullable
    String title;

    boolean canBeInterrupted;

    @Nonnull
    @Singular
    Set<LaunchState> displacementOptions;

    boolean rollback;

    @Nonnull
    @Override
    public String getId() {
        return id;
    }

    public static StoredStage of(Stage stage) {
        return new StoredStage(stage.getId(), stage.getTitle(), stage.isCanBeInterrupted(),
                stage.getDisplacementOptions(), stage.isRollback());
    }
}
