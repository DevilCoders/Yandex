package ru.yandex.ci.common.bazinga;

import lombok.Value;

import ru.yandex.misc.bender.annotation.BenderBindAllFields;

@Value
@BenderBindAllFields
public class StringUniqueIdentifier {
    String identifier;
}
