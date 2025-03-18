package ru.yandex.ci.storage.core.logbroker;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.core.logbroker.LogbrokerWriter;
import ru.yandex.ci.core.logbroker.LogbrokerWriterEmptyImpl;

@RequiredArgsConstructor
public class LogbrokerWriterFactoryEmptyImpl implements LogbrokerWriterFactory {

    @Nonnull
    private final String streamName;

    @Override
    public LogbrokerWriter create(int partition) {
        return new LogbrokerWriterEmptyImpl();
    }

    @Nonnull
    @Override
    public String getStreamName() {
        return streamName;
    }

}
