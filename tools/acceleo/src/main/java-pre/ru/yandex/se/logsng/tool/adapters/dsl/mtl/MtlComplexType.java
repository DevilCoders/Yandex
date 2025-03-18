package ru.yandex.se.logsng.tool.adapters.dsl.mtl;

import ru.yandex.se.logsng.tool.adapters.dsl.types.ComplexType;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Field;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

import java.util.Collection;
import java.util.HashMap;

/**
 * Created by astelmak on 12.07.16.
 */
public abstract class MtlComplexType extends MtlType implements ComplexType {
    private final Collection<Field> fields;
    private final HashMap<String, Field> fieldByXSDName;

    protected MtlComplexType(TypeKey key, Collection<Field> fields) {
        super(key);

        this.fields = fields;

        this.fieldByXSDName = new HashMap<>(fields.size());

        for (Field f : fields)
            this.fieldByXSDName.put(f.getXSDName(), f);
    }

    @Override
    public Collection<Field> getFields() {
        return fields;
    }

    @Override
    public Field getFieldByXSDName(String xsdName) {
        return fieldByXSDName.get(xsdName);
    }

    @Override
    public boolean isSimple() {
        return false;
    }
}
