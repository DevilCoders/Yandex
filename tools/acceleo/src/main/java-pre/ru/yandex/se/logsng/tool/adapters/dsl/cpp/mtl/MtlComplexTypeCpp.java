package ru.yandex.se.logsng.tool.adapters.dsl.cpp.mtl;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EStructuralFeature;
import ru.yandex.se.logsng.tool.adapters.dsl.mtl.MtlComplexType;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Field;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

import java.util.ArrayList;
import java.util.Collection;

/**
 * Created by astelmak on 06.06.16.
 */
public class MtlComplexTypeCpp extends MtlComplexType {
    private final String name;

    protected MtlComplexTypeCpp(TypeKey key, String name, Collection<Field> fields) {
        super(key, fields);

        this.name = name;
    }

    public static MtlComplexType getInstance(EClass clazz) {
        String xsdName = MtlTypeCpp.getXSDTypeName(clazz);
        String module = MtlTypeCpp.getModuleName(clazz);

        TypeKey key = new TypeKey(xsdName, module);

        MtlComplexTypeCpp result = (MtlComplexTypeCpp) MtlTypeCpp.types.get(key);

        if (result == null) {
            ArrayList<Field> fields = new ArrayList<>();

            for (EStructuralFeature f : clazz.getEStructuralFeatures())
                fields.add(new MtlFieldCpp(MtlTypeCpp.getXSDTypeName(f), f.getName(), MtlTypeCpp.getInstance(f), MtlTypeCpp.isOptional(f)));

            result = new MtlComplexTypeCpp(key, clazz.getName(), fields);

            MtlTypeCpp.types.put(key, result);
        }

        return result;
    }

    @Override
    public String getTypeName() {
        return name;
    }
}
