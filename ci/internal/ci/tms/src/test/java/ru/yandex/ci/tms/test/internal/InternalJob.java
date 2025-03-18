package ru.yandex.ci.tms.test.internal;

import java.util.Objects;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicReference;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.core.test.resources.StringResource;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

@RequiredArgsConstructor
@ExecutorInfo(
        title = "Create ticket in Yandex Tracker",
        description = "Создание релизного тикета в Трекере"
)
@Produces(single = {StringResource.class})
public class InternalJob implements JobExecutor {

    public static final UUID ID = UUID.fromString("26d78e1a-759d-412e-bb07-652f726c548a");

    private final Parameters parameters;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) {
        var value = StringResource.newBuilder()
                .setString(Objects.requireNonNullElse(parameters.stringValue.get(), ""))
                .build();
        context.resources().produce(value);
    }

    public static class Parameters {
        private final AtomicReference<String> stringValue = new AtomicReference<>();

        public AtomicReference<String> getStringValue() {
            return stringValue;
        }
    }
}
