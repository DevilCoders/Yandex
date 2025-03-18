package ru.yandex.se.logsng.tool.adapters.dsl.types;

import java.util.Set;

/**
 * Created by astelmak on 27.06.16.
 */
public interface Enum extends Type {
    Set<String> getEnumLiterals();
}
