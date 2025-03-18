package ru.yandex.se.logsng.tool.adapters.dsl.mtl;

/**
 * Created by astelmak on 12.07.16.
 */

import org.eclipse.emf.ecore.EAnnotation;
import org.eclipse.emf.ecore.EModelElement;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Type;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

import java.util.HashMap;

/**
 * Created by astelmak on 06.06.16.
 */
public abstract class MtlType implements Type {
    private final TypeKey key;

    protected MtlType(TypeKey key) {
        this.key = key;
    }

    @Override
    public TypeKey getKey() {
        return key;
    }

    @Override
    public String getRootTypeName() {
        return getTypeName();
    }

    public static String getModuleName(EModelElement el) {
        EAnnotation an = el.getEAnnotation("scarab:builtin:module");
        if (an == null)
            return null;
        return an.getDetails().get("appinfo");
    }

    public static String getXSDTypeName(EModelElement el) {
        return el.getEAnnotation("http:///org/eclipse/emf/ecore/util/ExtendedMetaData").getDetails().get("name");
    }

    public static boolean isOptional(EModelElement el) {
        String s = el.getEAnnotation("http:///org/eclipse/emf/ecore/util/ExtendedMetaData").getDetails().get("nillable");
        return s != null && "true".equalsIgnoreCase(s);
    }

    @Override
    public int compareTo(Type other) {
        return key.compareTo(other.getKey());
    }

    @Override
    public boolean equals(Object other) {
        if (this == other)
            return true;

        if (other == null || !(other instanceof Type))
            return false;

        Type type = (Type) other;

        return key.equals(type.getKey());
    }

    @Override
    public int hashCode() {
        return key.hashCode();
    }

    @Override
    public String toString() {
        return key.toString();
    }

    protected static final HashMap<TypeKey, Type> types = new HashMap<>();

    public static Type getTypeByKey(TypeKey key) {
        return types.get(key);
    }
}
