package ru.yandex.ci.core.logbroker;

import lombok.Value;

@Value
public class LogbrokerConfiguration {
    LogbrokerProxyBalancerHolder proxyHolder;
    LogbrokerTopics topics;
    LogbrokerProperties properties;
    LogbrokerCredentialsProvider credentialsProvider;
}
