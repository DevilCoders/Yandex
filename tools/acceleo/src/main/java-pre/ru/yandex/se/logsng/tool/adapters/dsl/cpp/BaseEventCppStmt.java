package ru.yandex.se.logsng.tool.adapters.dsl.cpp;

import ru.yandex.se.logsng.tool.adapters.dsl.cpp.mtl.MtlEventCpp;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.ObjStatement;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.StatementVisitor;
import ru.yandex.se.logsng.tool.adapters.dsl.types.ComplexType;

/**
 * Created by astelmak on 06.07.16.
 */
public class BaseEventCppStmt implements ObjStatement {
    private final MtlEventCpp event;

    public BaseEventCppStmt(MtlEventCpp event) {
        this.event = event;
    }

    @Override
    public ComplexType getType() {
        return event;
    }

    @Override
    public void accept(StatementVisitor visitor) {
        visitor.visit(this);
    }

    @Override
    public StringBuilder toString(StringBuilder b) {
        return b.append("event");
    }
}
