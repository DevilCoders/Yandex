package ru.yandex.se.logsng.tool.adapters.dsl.java.mtl;

import ru.yandex.se.logsng.tool.adapters.dsl.base.Util;
import ru.yandex.se.logsng.tool.adapters.dsl.mtl.MtlField;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Type;

/**
 * Created by astelmak on 13.07.16.
 */
public class MtlFieldJava extends MtlField {
    public MtlFieldJava(String xsdName, String name, Type type, boolean optional) {
        super(xsdName, name, type, optional);
    }

    @Override
    public String getter() {
        return Util.generateName(getXSDName(), Util.CASE.LOWER).toString();
    }
}
