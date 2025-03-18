package ru.yandex.se.logsng.tool.adapters.dsl.cpp.mtl;

import ru.yandex.se.logsng.tool.adapters.dsl.base.Util;
import ru.yandex.se.logsng.tool.adapters.dsl.mtl.MtlField;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Type;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

/**
 * Created by astelmak on 06.06.16.
 */
public class MtlFieldCpp extends MtlField {
    private final Type typeProxy = new Type() {
        @Override
        public int compareTo(Type o) {
            return getBaseType().compareTo(o);
        }

        @Override
        public TypeKey getKey() {
            return getBaseType().getKey();
        }

        @Override
        public String getTypeName() {
            final StringBuilder b;
            final String typeName = getBaseType().getTypeName();

            if (isOptional())
                b = new StringBuilder("TMaybe<").append(typeName).append('>');
            else {
                b = new StringBuilder(typeName);
                if (!isSimple())
                    b.append('&');
            }

            return b.toString();
        }

        @Override
        public String getRootTypeName() {
            return getBaseType().getRootTypeName();
        }

        @Override
        public boolean isSimple() {
            return getBaseType().isSimple();
        }

        @Override
        public String toString() {
            return getBaseType().toString();
        }

        @Override
        public int hashCode() {
            return getBaseType().hashCode();
        }

        @Override
        public boolean equals(Object obj) {
            return getBaseType().equals(obj);
        }
    };

    public MtlFieldCpp(String xsdName, String name, Type type, boolean optional) {
        super(xsdName, name, type, optional);
    }

    @Override
    public String getter() {
        return "Get" + Util.generateName(getName(), Util.CASE.UPPER);
    }

    @Override
    public Type getType() {
        return typeProxy;
    }

    Type getBaseType() {
        return super.getType();
    }
}
