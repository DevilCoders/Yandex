package ru.yandex.ci.core.resources;

import ru.yandex.ci.core.job.JobResourceType;

public interface HasResourceType {
    JobResourceType getResourceType();
}
