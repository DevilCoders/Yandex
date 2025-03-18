package ru.yandex.se.logsng.tool.adapters.dsl.statements;

import ru.yandex.se.logsng.tool.adapters.dsl.types.ComplexType;

/**
 * Created by astelmak on 24.05.16.
 */
public interface ObjStatement extends Value {
    ComplexType getType(); // event type code
}
