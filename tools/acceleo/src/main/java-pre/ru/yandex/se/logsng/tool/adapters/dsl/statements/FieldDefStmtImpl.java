package ru.yandex.se.logsng.tool.adapters.dsl.statements;

/**
 * Created by astelmak on 25.05.16.
 */
public class FieldDefStmtImpl implements FieldDefStatement {
    private final ObjStatement obj;
    private final Object[] params;

    public FieldDefStmtImpl(ObjStatement obj, Object ... params) {
        this.obj = obj;
        this.params = params;
    }

    @Override
    public void accept(StatementVisitor visitor) {
        getObj().accept(visitor);

        for (Object obj : params) {
            if (obj instanceof Statement)
                ((Statement)obj).accept(visitor);
        }

        visitor.visit(this);
    }

    @Override
    public StringBuilder toString(StringBuilder b) {
        return b;
    }

    @Override
    public ObjStatement getObj() {
        return obj;
    }

    @Override
    public Object[] getParams() {
        return params;
    }
}
