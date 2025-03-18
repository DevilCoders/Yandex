package ru.yandex.se.logsng.tool.adapters.dsl.statements;

/**
 * Created by astelmak on 25.05.16.
 */
public interface StatementVisitor {
    void visit(Statement st);

    void visit(FieldDefStatement fieldGetter);

    void visit(InterfaceAdapterStatement iAdapter);

    void visit(TypeDefStatement fieldDef);

    void visit(NothingStatement nothing);

    void visit(ObjStatement obj);
}
