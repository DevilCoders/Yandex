package ru.yandex.ci.engine.config.validation;

import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import lombok.Value;

import ru.yandex.ci.core.config.a.model.JobMultiplyConfig;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.definition.job.AbstractJob;
import ru.yandex.ci.flow.engine.definition.job.Job;
import ru.yandex.ci.flow.engine.definition.job.JobProperties;
import ru.yandex.ci.flow.engine.runtime.state.model.ExecutorContext;
import ru.yandex.ci.flow.engine.source_code.model.ConsumedResource;
import ru.yandex.ci.flow.engine.source_code.model.JobExecutorObject;
import ru.yandex.ci.flow.engine.source_code.model.ProducedResource;

/**
 * Проекция джобы с информацией о производимых и потребляемых ресурсах,
 * нужна для валидации входов/выходов джоб (@see {@link InputOutputTaskValidator}).
 */
@Value
public class ValidationJob implements AbstractJob<ValidationJob> {
    String id;
    List<Resource> staticResources;
    JobExecutorObject jobExecutorObject;
    Set<UpstreamLink<ValidationJob>> upstreams;
    JobMultiplyConfig jobMultiply;
    JobProperties jobProperties;
    ExecutorContext executorContext;

    @Override
    public String getId() {
        return id;
    }

    @Override
    public Set<UpstreamLink<ValidationJob>> getUpstreams() {
        return upstreams;
    }

    void addAllUpstreams(Set<UpstreamLink<ValidationJob>> upstreams) {
        this.upstreams.addAll(upstreams);
    }

    Collection<ProducedResource> getProducedResources() {
        return getJobExecutorObject().getProducedResources().values();
    }

    Collection<ConsumedResource> getConsumedResources() {
        return getJobExecutorObject().getConsumedResources().values();
    }

    static ValidationJob of(Job job, JobExecutorObject jobExecutorObject, ExecutorContext executorContext) {
        return new ValidationJob(
                job.getId(),
                job.getStaticResources(),
                jobExecutorObject,
                new HashSet<>(),
                job.getMultiply(),
                job.getProperties(),
                executorContext
        );
    }
}
