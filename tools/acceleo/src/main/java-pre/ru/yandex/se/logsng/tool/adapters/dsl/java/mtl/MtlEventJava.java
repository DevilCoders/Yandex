package ru.yandex.se.logsng.tool.adapters.dsl.java.mtl;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EStructuralFeature;
import ru.yandex.se.logsng.tool.EventType;
import ru.yandex.se.logsng.tool.adapters.dsl.mtl.MtlType;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Event;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Field;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

import java.util.ArrayList;
import java.util.Collection;

/**
 * Created by astelmak on 13.07.16.
 */
public class MtlEventJava extends MtlComplexTypeJava implements Event {
    public MtlEventJava(TypeKey key, String name, Collection<Field> fields) {
        super(key, name, fields);
    }

    @Override
    public String getTypeCode() {
        return "EventType." + EventType.getEventTypeEnumName(getKey().getModule(), getKey().getXsdName());
    }

    public static Event getEventInstance(EClass clazz) {
        String xsdName = MtlType.getXSDTypeName(clazz);
        String module = MtlType.getModuleName(clazz);

        TypeKey key = new TypeKey(xsdName, module);

        MtlEventJava result = (MtlEventJava) types.get(key);

        if (result == null) {
            ArrayList<Field> fields = new ArrayList<>();

            for (EStructuralFeature f : clazz.getEStructuralFeatures())
                fields.add(new MtlFieldJava(MtlType.getXSDTypeName(f), f.getName(), MtlTypeJava.getInstance(f), MtlType.isOptional(f)));

            result = new MtlEventJava(key, clazz.getName(), fields);

            types.put(key, result);
        }

        return result;
    }

}
