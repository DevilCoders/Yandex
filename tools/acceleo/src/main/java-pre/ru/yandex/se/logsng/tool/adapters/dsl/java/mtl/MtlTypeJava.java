package ru.yandex.se.logsng.tool.adapters.dsl.java.mtl;

import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EStructuralFeature;
import ru.yandex.se.logsng.tool.adapters.dsl.mtl.MtlType;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Type;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

/**
 * Created by astelmak on 12.07.16.
 */
public class MtlTypeJava extends MtlType {
    private final String name;
    private final boolean simple;

    private MtlTypeJava(TypeKey key, String name) {
        super(key);

        switch (key.getXsdName()) {
            case "char":
            case "int":
            case "long":
            case "short":
            case "double":
            case "float":
                this.name = name;
                simple = true;
                break;
            case "time":
                this.name = "BigInteger";
                simple = true;
                break;
            case "string":
                this.name = "String";
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

        return (module == null || module.isEmpty() ? "" : (getPackage(module) + ".")) + name;
    }

    @Override
    public String getRootTypeName() {
        return name;
    }

    @Override
    public boolean isSimple() {
        return simple;
    }

    public static String getPackage(String namespace) {
        if (namespace == null)
            return null;

        StringBuilder b = new StringBuilder("ru.yandex.se.scarab.api");

        if (!namespace.isEmpty())
            b.append('.').append(namespace.toLowerCase());

        return b.toString();
    }

    public static Type getInstance(EStructuralFeature el) {
        String xsdName = MtlType.getXSDTypeName(el.getEType());
        String module = getModuleName(el.getEType());

        TypeKey key = new TypeKey(xsdName, module);

        Type result = types.get(key);
        if (result == null) {
            if (el.getEType() instanceof EEnum)
                result = new MtlEnumJava(key, (EEnum)el.getEType());
            else
                result = new MtlTypeJava(key, el.getEType().getName());

            types.put(key, result);
        }

        return result;
    }
}
