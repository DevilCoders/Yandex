package ru.yandex.se.logsng.tool.adapters.dsl.cpp.mtl;

import org.eclipse.emf.ecore.*;
import ru.yandex.se.logsng.tool.adapters.dsl.mtl.MtlType;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Type;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

import java.util.*;

/**
 * Created by astelmak on 06.06.16.
 */
public class MtlTypeCpp extends MtlType {
    private final String name;
    private final boolean simple;

    private MtlTypeCpp(TypeKey key, String name) {
        super(key);

        switch (key.getXsdName()) {
            case "byte":
            case "int":
            case "long":
            case "short":
            case "unsignedByte":
            case "unsignedInt":
            case "unsignedLong":
            case "unsignedShort":
            case "double":
            case "float":
                this.name = name;
                simple = true;
                break;
            case "time":
                this.name = "TInstant";
                simple = true;
                break;
            case "string":
                this.name = "TString";
                simple = false;
                break;
            default:
                this.name = name;
                simple = false;
                break;
        }
    }

    @Override
    public String getTypeName() {
        if (isSimple())
            return name;

        String module = getKey().getModule();

        return (module == null || module.isEmpty() ? "" : (getCppNamespace(module) + "::")) + 'T' + name;
    }

    @Override
    public String getRootTypeName() {
        return name;
    }

    @Override
    public boolean isSimple() {
        return simple;
    }

    public static String getCppNamespace(String namespace) {
        if (namespace == null)
            return null;

        StringBuilder b = new StringBuilder(namespace.length() + 1);
        b.append('N');

        if (!namespace.isEmpty()) {
            b.append(Character.toUpperCase(namespace.charAt(0)));

            for (int i = 1; i < namespace.length(); i++) {
                char c = namespace.charAt(i);

                if (c == '-')
                    c = '_';

                b.append(c);
            }
        }

        return b.toString();
    }

    public static Type getInstance(EStructuralFeature el) {
        String xsdName = MtlType.getXSDTypeName(el.getEType());
        String module = getModuleName(el.getEType());

        TypeKey key = new TypeKey(xsdName, module);

        Type result = types.get(key);
        if (result == null) {
            if (el.getEType() instanceof EEnum)
                result = new MtlEnumCpp(key, (EEnum)el.getEType());
            else
                result = new MtlTypeCpp(key, el.getEType().getName());

            types.put(key, result);
        }

        return result;
    }
}
