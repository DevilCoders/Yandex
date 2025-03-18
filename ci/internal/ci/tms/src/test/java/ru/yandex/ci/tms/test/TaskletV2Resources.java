package ru.yandex.ci.tms.test;

import ru.yandex.ci.client.taskletv2.TaskletV2TestServer;
import ru.yandex.ci.core.taskletv2.TaskletV2Metadata;

public class TaskletV2Resources {
    public static final TaskletV2Metadata.Description WOODCUTTER =
            TaskletV2Metadata.Description.of("internal", "woodcutter", "release");

    public static final TaskletV2Metadata.Description FURNITURE =
            TaskletV2Metadata.Description.of("internal", "furniture", "release");

    public static final TaskletV2Metadata.Description SIMPLE =
            TaskletV2Metadata.Description.of("internal", "simple", "release");

    public static final TaskletV2Metadata.Description SIMPLE_INVALID =
            TaskletV2Metadata.Description.of("internal", "simple", TaskletV2TestServer.SIMULATE_INVALID_VERSION);

    private TaskletV2Resources() {
    }
}
