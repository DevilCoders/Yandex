package ru.yandex.ci.flow.engine.definition;

import java.util.List;

import javax.annotation.Nonnull;

import lombok.Getter;
import lombok.ToString;

import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.job.Job;
import ru.yandex.ci.flow.engine.definition.stage.Stage;

/**
 * Группирующий класс, содержащий список джоб флоу и типы ресурсов,
 * которые запрашиваются у пользователя при запуске флоу.
 */
@Getter
@ToString
public class Flow {
    private final List<Job> jobs;
    private final List<Job> cleanupJobs;

    /**
     * Типы ресурсов, которые запрашиваются у пользователя на старте флоу.
     */
    private final List<Stage> stages;

    public Flow(
            @Nonnull FlowBuilder builder,
            @Nonnull List<Job> jobs,
            @Nonnull List<Job> cleanupJobs,
            @Nonnull List<Stage> stages
    ) {
        this.jobs = List.copyOf(jobs);
        this.cleanupJobs = List.copyOf(cleanupJobs);
        this.stages = List.copyOf(stages);
    }

    public List<Job> getJobs() {
        return jobs;
    }

    public List<Job> getCleanupJobs() {
        return cleanupJobs;
    }

    public List<Stage> getStages() {
        return stages;
    }
}
