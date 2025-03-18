package ru.yandex.se.logsng.tool.adapters.dsl.cpp;

import ru.yandex.se.logsng.tool.adapters.dsl.statements.FieldStatement;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.ObjStatement;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.StatementVisitor;

/**
 * Created by astelmak on 25.05.16.
 */
public abstract class FieldCppStmt implements FieldStatement {
    private final ObjStatement obj;
    private final String name;

    public FieldCppStmt(ObjStatement obj, String name) {
        this.obj = obj;
        this.name = name;
    }

    @Override
    public ObjStatement getObj() {
        return obj;
    }

    @Override
    public String getName() {
        return name;
    }

    @Override
    public String toString() {
        return toString(new StringBuilder()).toString();
    }

    @Override
    public StringBuilder toString(StringBuilder b) {
        obj.toString(b).append('.');

        return getter(b).append("()");
    }

    @Override
    public void accept(StatementVisitor visitor) {
        obj.accept(visitor);

        visitor.visit(this);
    }

    protected abstract StringBuilder getter(StringBuilder b);
}
