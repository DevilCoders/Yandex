package ru.yandex.ci.storage.core.message.main;

import java.util.List;

import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.utils.TimeTraceService;

public abstract class MainStreamMessageProcessor {
    public abstract void process(List<TaskMessages.TaskMessage> protoTaskMessages, TimeTraceService.Trace trace);
}
