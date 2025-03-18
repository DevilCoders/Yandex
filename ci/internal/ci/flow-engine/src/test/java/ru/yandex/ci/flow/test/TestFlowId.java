package ru.yandex.ci.flow.test;

import java.nio.file.Path;

import ru.yandex.ci.core.config.FlowFullId;

public class TestFlowId {
    public static final Path TEST_PATH = Path.of("/ci/test/a.yaml");

    private TestFlowId() {
    }

    public static FlowFullId of(String flowId) {
        return FlowFullId.of(TEST_PATH, flowId);
    }
}
