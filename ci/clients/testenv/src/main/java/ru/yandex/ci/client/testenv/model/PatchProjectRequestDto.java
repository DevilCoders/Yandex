package ru.yandex.ci.client.testenv.model;

import lombok.Value;

@Value
public class PatchProjectRequestDto {
    String status;
    String comment;
}
