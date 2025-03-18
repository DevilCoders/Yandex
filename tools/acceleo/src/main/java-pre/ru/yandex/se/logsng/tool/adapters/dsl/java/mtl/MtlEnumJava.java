package ru.yandex.se.logsng.tool.adapters.dsl.java.mtl;

import org.eclipse.emf.ecore.EEnum;
import ru.yandex.se.logsng.tool.adapters.dsl.mtl.MtlEnum;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

/**
 * Created by astelmak on 12.07.16.
 */
public class MtlEnumJava extends MtlEnum {
    private final String name;

    MtlEnumJava(TypeKey key, EEnum en) {
        super(key, en);

        this.name = en.getName();
    }

    @Override
    public String getTypeName() {
        String module = getKey().getModule();
        return (module == null || module.isEmpty() ? "" : (MtlTypeJava.getPackage(module) + '.')) + name;
    }

    @Override
    public boolean isSimple() {
        return true;
    }
}
