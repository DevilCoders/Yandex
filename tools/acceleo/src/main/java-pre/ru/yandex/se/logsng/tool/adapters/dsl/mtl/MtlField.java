package ru.yandex.se.logsng.tool.adapters.dsl.mtl;

import ru.yandex.se.logsng.tool.adapters.dsl.types.Field;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Type;

/**
 * Created by astelmak on 13.07.16.
 */
public abstract class MtlField implements Field {
    private final String xsdName, name;
    private final Type type;
    private final boolean optional;

    public MtlField(String xsdName, String name, Type type, boolean optional) {
        this.xsdName = xsdName;
        this.name = name;
        this.type = type;
        this.optional = optional;
    }

    @Override
    public String getXSDName() {
        return xsdName;
    }

    @Override
    public String getName() {
        return name;
    }

    @Override
    public boolean isOptional() {
        return optional;
    }

    @Override
    public Type getType() {
        return type;
    }
}

