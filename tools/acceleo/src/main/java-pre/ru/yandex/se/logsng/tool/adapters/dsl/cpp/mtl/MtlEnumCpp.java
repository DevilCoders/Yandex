package ru.yandex.se.logsng.tool.adapters.dsl.cpp.mtl;

import org.eclipse.emf.ecore.EEnum;
import ru.yandex.se.logsng.tool.adapters.dsl.mtl.MtlEnum;
import ru.yandex.se.logsng.tool.adapters.dsl.types.TypeKey;

/**
 * Created by astelmak on 28.06.16.
 */
public class MtlEnumCpp extends MtlEnum {
    private final String name;

    MtlEnumCpp(TypeKey key, EEnum en) {
        super(key, en);

        this.name = en.getName();
    }

    @Override
    public String getTypeName() {
        String module = getKey().getModule();
        return (module == null || module.isEmpty() ? "" : (MtlTypeCpp.getCppNamespace(module) + "::")) + 'E' + name;
    }

    @Override
    public boolean isSimple() {
        return true;
    }
}
