package ru.yandex.ci.observer.api.stress_test;

import lombok.Value;

@Value(staticConstructor = "of")
public class CheckRevisionsDto {
    OrderedRevisionDto left;
    OrderedRevisionDto right;
    Long diffSetId;
}
