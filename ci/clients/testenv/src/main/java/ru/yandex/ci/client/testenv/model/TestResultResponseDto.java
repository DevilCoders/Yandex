package ru.yandex.ci.client.testenv.model;

import java.util.List;

import lombok.Value;

@Value
public class TestResultResponseDto {
    List<TestResultDto> rows;
}
