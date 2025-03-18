package ru.yandex.se.logsng.tool.adapters.dsl.statements;

/**
 * Created by astelmak on 24.05.16.
 */
public interface Statement {
    void accept(StatementVisitor visitor);

    StringBuilder toString(StringBuilder b);
}
