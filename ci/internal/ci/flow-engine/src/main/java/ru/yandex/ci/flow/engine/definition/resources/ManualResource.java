package ru.yandex.ci.flow.engine.definition.resources;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.core.job.JobResourceType;

@Value(staticConstructor = "of")
public class ManualResource {
    @Nonnull
    JobResourceType resourceType;
}
