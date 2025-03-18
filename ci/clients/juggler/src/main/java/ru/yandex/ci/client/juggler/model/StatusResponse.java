package ru.yandex.ci.client.juggler.model;

import lombok.Value;

@Value
public class StatusResponse {
    int count;
    Status status;
}
