package ru.yandex.ci.client.juggler.model;

import java.util.List;

import lombok.Value;

@Value
public class ChecksStatusResponse {
    boolean responseTooLarge;
    List<StatusResponse> statuses;
}
