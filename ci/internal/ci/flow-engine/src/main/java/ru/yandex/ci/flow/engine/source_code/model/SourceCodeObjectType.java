package ru.yandex.ci.flow.engine.source_code.model;

public enum SourceCodeObjectType {
    RESOURCE,
    JOB_EXECUTOR,
    @Deprecated
    FLOW_SUBSCRIBER,
    @Deprecated
    RESOURCE_FIELD_CONTROL
}
