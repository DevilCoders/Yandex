package ru.yandex.ci.storage.reader.message.main;

import ru.yandex.ci.storage.core.cache.SettingsCache;
import ru.yandex.ci.storage.core.message.main.MainStreamMessageProcessor;
import ru.yandex.ci.storage.core.message.main.MainStreamStatistics;
import ru.yandex.ci.storage.core.utils.TimeTraceService;

public class MainStreamReadProcessor extends ru.yandex.ci.storage.core.message.main.MainStreamReadProcessor {
    private final SettingsCache settingsCache;

    public MainStreamReadProcessor(
            TimeTraceService timeTraceService,
            MainStreamMessageProcessor messageProcessor,
            SettingsCache settingsCache,
            MainStreamStatistics statistics
    ) {
        super(timeTraceService, messageProcessor, statistics);

        this.settingsCache = settingsCache;
    }

    @Override
    protected boolean skipParseErrors() {
        return settingsCache.get().getReader().getSkip().isParseError();
    }
}
