package ru.yandex.ci.client.juggler.model;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonInclude;
import lombok.Value;

@Value
@JsonInclude(JsonInclude.Include.NON_EMPTY)
public class ChecksStatusRequest {
    List<FilterRequest> filters;
    int limit;
}
