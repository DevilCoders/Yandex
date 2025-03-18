package ru.yandex.ci.engine.autocheck.jobs.large;

import java.util.UUID;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.client.storage.StorageApiClient;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.definition.resources.Produces;
import ru.yandex.ci.storage.api.StorageApi;

@RequiredArgsConstructor
@ExecutorInfo(
        title = "Start Large tests in single flow",
        description = "Internal job for starting Large tests task"
)
@Consume(name = "request", proto = StorageApi.GetLargeTaskRequest.class)
@Produces(single = StorageApi.GetLargeTaskResponse.class)
public class StartLargeTestsJob implements JobExecutor {

    private static final UUID ID = UUID.fromString("e2594c91-9563-427b-89df-a0f51042f009");

    private final StorageApiClient storageApiClient;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var request = context.resources().consume(StorageApi.GetLargeTaskRequest.class);
        var response = storageApiClient.getLargeTask(request);
        context.resources().produce(Resource.of(response, "response"));
    }
}
