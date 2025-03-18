package ru.yandex.ci.common.bazinga.monitoring;

import lombok.Value;

import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@Value
@BenderBindAllFields
public class Person {
    String name;
}
