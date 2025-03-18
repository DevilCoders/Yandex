package ru.yandex.se.logsng.tool.adapters;

import org.eclipse.emf.ecore.EClass;
import ru.yandex.se.logsng.tool.adapters.dsl.java.mtl.MtlEventJava;
import ru.yandex.se.logsng.tool.adapters.dsl.java.mtl.MtlInterfaceAdapterJava;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.ObjStatement;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.StatementVisitor;
import ru.yandex.se.logsng.tool.adapters.dsl.types.*;

import java.util.Collections;
import java.util.List;

/**
 * Created by astelmak on 13.07.16.
 */
public class MainJava extends Main {
    @Override
    protected InterfaceAdapter createAdapter(EClass adapter) {
        return MtlInterfaceAdapterJava.getInstance(adapter);
    }

    @Override
    protected Event getEvent(EClass clazz) {
        return MtlEventJava.getEventInstance(clazz);
    }

    @Override
    protected ObjStatement createBaseObject() {
        List<Field> fields = Collections.emptyList();
        final MtlEventJava baseEventType = new MtlEventJava(new TypeKey("Event", "common"), "event", fields) {
            @Override
            public boolean isSimple() {
                return true;
            }
        };

        return new ObjStatement() {
            @Override
            public ComplexType getType() {
                return baseEventType;
            }

            @Override
            public void accept(StatementVisitor visitor) {
                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                return b.append("event");
            }
        };
    }
}
