package ru.yandex.se.logsng.tool.adapters.dsl.cpp.mtl;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EStructuralFeature;
import ru.yandex.se.logsng.tool.adapters.dsl.mtl.MtlType;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Field;
import ru.yandex.se.logsng.tool.adapters.dsl.types.InterfaceAdapter;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

import java.util.ArrayList;
import java.util.Collection;

/**
 * Created by astelmak on 07.06.16.
 */
public class MtlInterfaceAdapterCpp extends MtlComplexTypeCpp implements InterfaceAdapter {

    protected MtlInterfaceAdapterCpp(TypeKey key, String name, Collection<Field> fields) {
        super(key, name, fields);
    }

    public static MtlInterfaceAdapterCpp getInstance(EClass clazz) {
        String xsdName = MtlTypeCpp.getXSDTypeName(clazz);
        String module = MtlTypeCpp.getModuleName(clazz);

        TypeKey key = new TypeKey(xsdName, module);

        MtlInterfaceAdapterCpp result = (MtlInterfaceAdapterCpp) MtlType.types.get(key);

        if (result == null) {
            ArrayList<Field> fields = new ArrayList<>();

            for (EStructuralFeature f : clazz.getEStructuralFeatures())
                fields.add(new MtlFieldCpp(MtlTypeCpp.getXSDTypeName(f), f.getName(), MtlTypeCpp.getInstance(f), MtlTypeCpp.isOptional(f)));

            result = new MtlInterfaceAdapterCpp(key, clazz.getName(), fields);

            MtlTypeCpp.types.put(key, result);
        }

        return result;
    }
}
