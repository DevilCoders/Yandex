package ru.yandex.se.logsng.tool.adapters.dsl.cpp.mtl;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EStructuralFeature;
import ru.yandex.se.logsng.tool.EventType;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Event;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Field;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

import java.util.ArrayList;
import java.util.Collection;

/**
 * Created by astelmak on 06.06.16.
 */
public class MtlEventCpp extends MtlComplexTypeCpp implements Event {
    public MtlEventCpp(TypeKey key, String name, Collection<Field> fields) {
        super(key, name, fields);
    }

    @Override
    public String getTypeCode() {
        return "TStringBuf::" + EventType.getEventTypeEnumName(getKey().getModule(), getKey().getXsdName());
    }

    @Override
    public String getTypeName() {
        String module = getKey().getModule();

        return (module == null || module.isEmpty() ? "" : (MtlTypeCpp.getCppNamespace(module) + "::")) + 'I' + super.getTypeName();
    }

    public static Event getEventInstance(EClass clazz) {
        String xsdName = MtlTypeCpp.getXSDTypeName(clazz);
        String module = MtlTypeCpp.getModuleName(clazz);

        TypeKey key = new TypeKey(xsdName, module);

        MtlEventCpp result = (MtlEventCpp) types.get(key);

        if (result == null) {
            ArrayList<Field> fields = new ArrayList<>();

            for (EStructuralFeature f : clazz.getEStructuralFeatures())
                fields.add(new MtlFieldCpp(MtlTypeCpp.getXSDTypeName(f), f.getName(), MtlTypeCpp.getInstance(f), MtlTypeCpp.isOptional(f)));

            result = new MtlEventCpp(key, clazz.getName(), fields);

            types.put(key, result);
        }

        return result;
    }

}
