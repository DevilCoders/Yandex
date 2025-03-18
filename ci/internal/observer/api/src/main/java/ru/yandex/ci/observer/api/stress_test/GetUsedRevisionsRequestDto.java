package ru.yandex.ci.observer.api.stress_test;

import java.util.List;

import lombok.Value;

@Value
public class GetUsedRevisionsRequestDto {

    List<String> rightRevisions;
    String namespace;

}
