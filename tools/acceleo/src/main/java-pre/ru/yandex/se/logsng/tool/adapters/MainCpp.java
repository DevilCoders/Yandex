package ru.yandex.se.logsng.tool.adapters;

import org.eclipse.emf.ecore.EClass;
import ru.yandex.se.logsng.tool.adapters.dsl.cpp.BaseEventCppStmt;
import ru.yandex.se.logsng.tool.adapters.dsl.cpp.mtl.MtlEventCpp;
import ru.yandex.se.logsng.tool.adapters.dsl.cpp.mtl.MtlInterfaceAdapterCpp;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.ObjStatement;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Event;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Field;
import ru.yandex.se.logsng.tool.adapters.dsl.types.InterfaceAdapter;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

import java.util.Collections;
import java.util.List;

/**
 * Created by astelmak on 12.07.16.
 */
public class MainCpp extends Main {
    @Override
    protected InterfaceAdapter createAdapter(EClass adapter) {
        return MtlInterfaceAdapterCpp.getInstance(adapter);
    }

    @Override
    protected Event getEvent(EClass clazz) {
        return MtlEventCpp.getEventInstance(clazz);
    }

    @Override
    protected ObjStatement createBaseObject() {
        List<Field> fields = Collections.emptyList();
        final MtlEventCpp baseEventType = new MtlEventCpp(new TypeKey("IEvent", "common"), "event", fields) {
            @Override
            public boolean isSimple() {
                return true;
            }
        };

        return new BaseEventCppStmt(baseEventType);
    }
}
