package ru.yandex.se.logsng.tool.adapters.dsl.types;

/**
 * Created by astelmak on 07.07.16.
 */
public class TypeKey implements Comparable<TypeKey> {
    private final String xsdName, module;

    public TypeKey(String xsdName, String module) {
        this.xsdName = xsdName;
        this.module = module;
    }

    public String getXsdName() {
        return xsdName;
    }

    public String getModule() {
        return module;
    }

    @Override
    public boolean equals(Object other) {
        if (this == other)
            return true;

        if (other == null || !(other instanceof TypeKey))
            return false;

        TypeKey typeKey = (TypeKey) other;

        return (xsdName == typeKey.xsdName || (xsdName != null && xsdName.equals(typeKey.xsdName))) &&
                (module == typeKey.module || (module != null && module.equals(typeKey.module)));
    }

    @Override
    public int hashCode() {
        return 31 * (xsdName != null ? xsdName.hashCode() : 0) + (module != null ? module.hashCode() : 0);
    }

    @Override
    public int compareTo(TypeKey other) {
        if (xsdName == null) {
            if (other.xsdName != null)
                return 1;
        }
        else {
            if (other.xsdName == null)
                return -1;

            int result = xsdName.compareTo(other.xsdName);
            if (result != 0)
                return result;
        }

        if (module == null) {
            if (other.module != null)
                return 1;
        }
        else {
            if (other.module == null)
                return -1;

            return module.compareTo(other.module);
        }

        return 0;
    }

    @Override
    public String toString() {
        return module + '.' + xsdName;
    }
}
