package ru.yandex.ci.client.testenv.model;

import javax.annotation.Nullable;

import lombok.Value;

@Value
public class StartCheckV2ResponseDto {

    @Nullable
    String checkId;
    @Nullable
    String fastCircuitCheckId;
    @Nullable
    String message;

}
