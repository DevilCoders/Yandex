package ru.yandex.se.logsng.tool.adapters.dsl.types;

/**
 * Created by astelmak on 26.05.16.
 */
public interface Type extends Comparable<Type> {
    TypeKey getKey();

    String getTypeName();

    String getRootTypeName();

    boolean isSimple();
}
