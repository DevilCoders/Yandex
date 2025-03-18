package ru.yandex.se.logsng.tool.adapters.dsl.statements;

/**
 * Created by astelmak on 16.06.16.
 */
public interface FieldStatement extends Value {
    ObjStatement getObj();

    String getName();
}
