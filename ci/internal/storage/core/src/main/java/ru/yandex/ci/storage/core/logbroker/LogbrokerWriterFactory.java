package ru.yandex.ci.storage.core.logbroker;

import ru.yandex.ci.core.logbroker.LogbrokerWriter;

public interface LogbrokerWriterFactory {

    LogbrokerWriter create(int partition);

    String getStreamName();

}
