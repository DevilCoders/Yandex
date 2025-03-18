package ru.yandex.ci.client.observer;

import java.util.List;

import lombok.Value;

@Value
public class GetUsedRevisionsRequestDto {

    List<String> rightRevisions;
    String namespace;

}
