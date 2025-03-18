package ru.yandex.ci.client.juggler.model;

import java.util.List;

import lombok.Value;

@Value
public class MutesResponse {
    int total;
    List<MuteInfoResponse> items;
}
