package ru.yandex.se.logsng.tool.adapters.dsl.java.mtl;

import ru.yandex.se.logsng.tool.adapters.dsl.mtl.MtlComplexType;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Field;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

import java.util.Collection;

/**
 * Created by astelmak on 13.07.16.
 */
public class MtlComplexTypeJava extends MtlComplexType {
    protected final String name;

    protected MtlComplexTypeJava(TypeKey key, String name, Collection<Field> fields) {
        super(key, fields);

        this.name = name;
    }

    @Override
    public String getTypeName() {
        String module = getKey().getModule();

        return (module == null || module.isEmpty() ? "" : (MtlTypeJava.getPackage(module) + ".")) + name;
    }
}
