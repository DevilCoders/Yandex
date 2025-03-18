package ru.yandex.se.logsng.tool.adapters.dsl.statements;

import ru.yandex.se.logsng.tool.adapters.dsl.types.ComplexType;

/**
 * Created by astelmak on 16.06.16.
 */
public interface TypeDefStatement extends Statement {
    ComplexType getType();

    Object getDefinition();
}
