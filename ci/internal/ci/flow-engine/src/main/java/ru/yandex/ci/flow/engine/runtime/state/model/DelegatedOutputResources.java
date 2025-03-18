package ru.yandex.ci.flow.engine.runtime.state.model;

import javax.annotation.Nonnull;

import lombok.Value;

import ru.yandex.ci.core.job.JobInstance;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value(staticConstructor = "of")
public class DelegatedOutputResources {
    @Nonnull
    ResourceRefContainer resourceRef;

    @Nonnull
    JobInstance.Id jobOutput;

    public DelegatedOutputResources mergeWith(ResourceRefContainer resourceRef) {
        return DelegatedOutputResources.of(this.resourceRef.mergeWith(resourceRef), jobOutput);
    }
}
