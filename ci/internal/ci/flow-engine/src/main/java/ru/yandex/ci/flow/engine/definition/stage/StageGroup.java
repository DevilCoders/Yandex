package ru.yandex.ci.flow.engine.definition.stage;

import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import lombok.Getter;
import lombok.ToString;

/**
 * Сущность, которая группирует стадии и задаёт их порядок. В одном флоу можно использовать только стадии одной
 * группы. Также в флоу нельзя использовать группы в порядке, который не соответствует порядку следования стадий
 * в группе. Однако разрешается пропускать какую-либо стадию, например hotfix-флоу может пропустить выкладку
 * в prestable.
 */
@ToString
@Getter
public class StageGroup {
    @Nonnull
    private final List<Stage> stages;

    public StageGroup(String... stageIds) {
        this(Arrays.stream(stageIds).map(StageBuilder::create).collect(Collectors.toList()));
    }

    public StageGroup(Collection<String> stageIds) {
        this(stageIds.stream().map(StageBuilder::create).collect(Collectors.toList()));
    }

    public StageGroup(StageBuilder... stageBuilders) {
        this(Arrays.asList(stageBuilders));
    }

    public StageGroup(List<StageBuilder> stageBuilders) {
        stages = stageBuilders.stream().map(b -> b.withStageGroup(this).build()).collect(Collectors.toList());
    }

    public Stage getStage(String name) {
        return this.stages.stream()
                .filter(s -> s.getId().equals(name))
                .findFirst()
                .orElseThrow(() -> new NoSuchElementException("stage " + name + " not found in " + this));
    }
}
