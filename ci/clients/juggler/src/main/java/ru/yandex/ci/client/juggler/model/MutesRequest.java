package ru.yandex.ci.client.juggler.model;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.Value;

@Value
@JsonInclude(JsonInclude.Include.NON_EMPTY)
public class MutesRequest {
    List<MuteFilterRequest> filters;
    int page;
    int pageSize;
    boolean includeExpired;
}
