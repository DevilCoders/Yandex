package ru.yandex.se.logsng.tool.adapters.dsl.java;

import ru.yandex.se.logsng.tool.adapters.dsl.statements.InterfaceAdapterStatement;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.Statement;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.StatementVisitor;

/**
 * Created by astelmak on 12.07.16.
 */
public class InterfaceAdapterJavaStmt implements InterfaceAdapterStatement {
    private final String name;
    private final Statement[] params;

    public InterfaceAdapterJavaStmt(String name, Statement[] params) {
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
        return b.append("provide").append(getName()).append("(event)");
    }

    public Statement[] getParams() {
        return params;
    }

    public String getName() {
        return name;
    }
}
