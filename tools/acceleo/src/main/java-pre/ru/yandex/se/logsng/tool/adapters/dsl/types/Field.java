package ru.yandex.se.logsng.tool.adapters.dsl.types;

/**
 * Created by astelmak on 26.05.16.
 */
public interface Field {
    String getXSDName();

    String getName();

    String getter();

    Type getType();

    boolean isOptional();
}
