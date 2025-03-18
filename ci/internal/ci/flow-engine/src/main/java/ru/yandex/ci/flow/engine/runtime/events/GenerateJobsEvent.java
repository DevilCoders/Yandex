package ru.yandex.ci.flow.engine.runtime.events;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.flow.engine.runtime.JobContextFactory;

/**
 * Generates multiple jobs based on given example, each with it's own resources.
 */
@Value
public class GenerateJobsEvent implements FlowEvent {
    @Nonnull JobContextFactory jobContextFactory;
    @Nonnull String jobId;
}
