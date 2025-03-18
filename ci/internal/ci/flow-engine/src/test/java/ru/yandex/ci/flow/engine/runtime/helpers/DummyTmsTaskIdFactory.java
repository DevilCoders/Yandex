package ru.yandex.ci.flow.engine.runtime.helpers;

import java.util.UUID;

import ru.yandex.ci.flow.engine.runtime.TmsTaskId;
import ru.yandex.commune.bazinga.impl.FullJobId;
import ru.yandex.commune.bazinga.impl.JobId;
import ru.yandex.commune.bazinga.impl.TaskId;

public class DummyTmsTaskIdFactory {

    private DummyTmsTaskIdFactory() {
    }


    public static TmsTaskId create() {
        return TmsTaskId.fromBazingaId(new FullJobId(new TaskId("flow"), new JobId(UUID.randomUUID())));
    }
}
