package ru.yandex.ci.flow.engine.definition.stage;

import java.util.HashSet;
import java.util.Set;

import javax.annotation.Nonnull;

import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchState;

public class StageBuilder {
    private StageGroup stageGroup;
    @Nonnull
    private final String id;
    private String title;
    private boolean canBeInterrupted = true;
    private final Set<LaunchState> displacementOptions = new HashSet<>();
    private boolean rollback;

    private StageBuilder(@Nonnull String id) {
        this.id = id;
    }

    public static StageBuilder create(String id) {
        return new StageBuilder(id);
    }

    public StageBuilder withStageGroup(StageGroup stageGroup) {
        this.stageGroup = stageGroup;
        return this;
    }

    public StageBuilder withTitle(String title) {
        this.title = title;
        return this;
    }

    public StageBuilder uninterruptable() {
        this.canBeInterrupted = false;
        return this;
    }

    public StageBuilder interruptable() {
        this.canBeInterrupted = true;
        return this;
    }

    public StageBuilder withRollback(boolean rollback) {
        this.rollback = rollback;
        return this;
    }

    public StageBuilder withCanBeInterrupted(boolean canBeInterrupted) {
        this.canBeInterrupted = canBeInterrupted;
        return this;
    }

    public StageBuilder withDisplacementOptions(Set<Status> statuses) {
        statuses.stream()
                .map(status -> LaunchState.lookup(status).orElseThrow(() ->
                        new IllegalStateException("Unable to find state " + status)))
                .forEach(displacementOptions::add);
        return this;
    }

    public Stage build() {
        return new Stage(stageGroup, id, title, canBeInterrupted, displacementOptions, rollback);
    }
}
