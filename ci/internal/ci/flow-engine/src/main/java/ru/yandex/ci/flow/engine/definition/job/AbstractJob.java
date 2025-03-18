package ru.yandex.ci.flow.engine.definition.job;

import java.util.Set;

import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;

public interface AbstractJob<JOB> {
    String getId();

    Set<UpstreamLink<JOB>> getUpstreams();
}
