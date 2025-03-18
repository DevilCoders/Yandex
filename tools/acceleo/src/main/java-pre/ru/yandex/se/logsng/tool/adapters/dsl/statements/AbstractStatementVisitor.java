package ru.yandex.se.logsng.tool.adapters.dsl.statements;


/**
 * Created by astelmak on 03.06.16.
 */
public class AbstractStatementVisitor implements StatementVisitor {
    @Override
    public void visit(Statement st) {
    }

    @Override
    public void visit(FieldDefStatement fieldGetter) {
    }

    @Override
    public void visit(InterfaceAdapterStatement iAdapter) {
    }

    @Override
    public void visit(TypeDefStatement fieldDef) {
    }

    @Override
    public void visit(NothingStatement nothing) {
    }

    @Override
    public void visit(ObjStatement obj) {
    }
}
