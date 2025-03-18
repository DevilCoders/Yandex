package ru.yandex.ci.core.logbroker;

import lombok.Value;

@Value
public class LogbrokerProperties {
    String host;
    String consumer;
    int logbrokerTvmId;
}
