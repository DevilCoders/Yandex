package ru.yandex.ci.storage.tests.logbroker;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.core.logbroker.LogbrokerWriter;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;

@RequiredArgsConstructor
public class TestLogbrokerWriterFactoryImpl implements LogbrokerWriterFactory {

    @Nonnull
    private final String topic;

    @Nonnull
    private final String streamName;

    @Nonnull
    private final TestLogbrokerService logbrokerService;

    @Override
    public LogbrokerWriter create(int partition) {
        return new TestLogbrokerWriterImpl(topic, "some-source-id", logbrokerService);
    }

    @Nonnull
    @Override
    public String getStreamName() {
        return streamName;
    }

}
