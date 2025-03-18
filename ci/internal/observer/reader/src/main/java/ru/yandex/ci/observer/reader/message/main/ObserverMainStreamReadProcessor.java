package ru.yandex.ci.observer.reader.message.main;

import ru.yandex.ci.storage.core.message.main.MainStreamMessageProcessor;
import ru.yandex.ci.storage.core.message.main.MainStreamReadProcessor;
import ru.yandex.ci.storage.core.message.main.MainStreamStatistics;
import ru.yandex.ci.storage.core.utils.TimeTraceService;

public class ObserverMainStreamReadProcessor extends MainStreamReadProcessor {
    public ObserverMainStreamReadProcessor(
            TimeTraceService timeTraceService,
            MainStreamMessageProcessor messageProcessor,
            MainStreamStatistics statistics
    ) {
        super(timeTraceService, messageProcessor, statistics);
    }

    @Override
    protected boolean skipParseErrors() {
        return true;
    }
}
