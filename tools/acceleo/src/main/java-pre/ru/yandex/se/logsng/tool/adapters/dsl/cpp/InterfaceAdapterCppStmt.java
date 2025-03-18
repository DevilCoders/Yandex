package ru.yandex.se.logsng.tool.adapters.dsl.cpp;

import ru.yandex.se.logsng.tool.adapters.dsl.statements.InterfaceAdapterStatement;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.Statement;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.StatementVisitor;

/**
 * Created by astelmak on 26.05.16.
 */
public class InterfaceAdapterCppStmt implements InterfaceAdapterStatement {
    private final String name;
    private final Statement[] params;

    public InterfaceAdapterCppStmt(String name, Statement[] params) {
        this.name = name;
        this.params = params;
    }

    @Override
    public void accept(StatementVisitor visitor) {
        for (Statement v : params)
            v.accept(visitor);
        visitor.visit(this);
    }

    @Override
    public StringBuilder toString(StringBuilder b) {
        return b.append("Create").append(getName()).append("(evContainer)");
    }

    public Statement[] getParams() {
        return params;
    }

    @Override
    public String toString() {
        return toString(new StringBuilder()).toString();
    }

    public String getName() {
        return name;
    }
}
