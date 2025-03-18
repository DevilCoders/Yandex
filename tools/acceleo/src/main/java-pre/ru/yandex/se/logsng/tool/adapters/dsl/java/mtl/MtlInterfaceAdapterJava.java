package ru.yandex.se.logsng.tool.adapters.dsl.java.mtl;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EStructuralFeature;
import ru.yandex.se.logsng.tool.adapters.dsl.mtl.MtlType;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Field;
import ru.yandex.se.logsng.tool.adapters.dsl.types.InterfaceAdapter;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

import java.util.ArrayList;
import java.util.Collection;

/**
 * Created by astelmak on 13.07.16.
 */
public class MtlInterfaceAdapterJava extends MtlComplexTypeJava implements InterfaceAdapter {
    protected MtlInterfaceAdapterJava(TypeKey key, String name, Collection<Field> fields) {
        super(key, name, fields);
    }

    public static MtlInterfaceAdapterJava getInstance(EClass clazz) {
        String xsdName = MtlType.getXSDTypeName(clazz);
        String module = MtlType.getModuleName(clazz);

        TypeKey key = new TypeKey(xsdName, module);

        MtlInterfaceAdapterJava result = (MtlInterfaceAdapterJava) MtlType.types.get(key);

        if (result == null) {
            ArrayList<Field> fields = new ArrayList<>();

            for (EStructuralFeature f : clazz.getEStructuralFeatures())
                fields.add(new MtlFieldJava(MtlType.getXSDTypeName(f), f.getName(), MtlTypeJava.getInstance(f), MtlType.isOptional(f)));

            result = new MtlInterfaceAdapterJava(key, clazz.getName(), fields);

            MtlType.types.put(key, result);
        }

        return result;
    }

    @Override
    public String getTypeName() {
        String module = getKey().getModule();

        return (module == null || module.isEmpty() ? "" : (getAdapterPackage(module) + ".")) + name;
    }

    @Override
    public String getRootTypeName() {
        return name;
    }

    private static String getAdapterPackage(String namespace) {
        if (namespace == null)
            return null;

        StringBuilder b = new StringBuilder("ru.yandex.se.scarab.api.common.adapter");

        return b.toString();
    }

}
