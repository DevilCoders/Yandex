package ru.yandex.ci.client.observer;

import lombok.Value;

@Value(staticConstructor = "of")
public class CheckRevisionsDto {
    OrderedRevisionDto left;
    OrderedRevisionDto right;
    Long diffSetId;
}
