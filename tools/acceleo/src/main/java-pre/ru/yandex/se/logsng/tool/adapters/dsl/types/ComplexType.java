package ru.yandex.se.logsng.tool.adapters.dsl.types;

import java.util.Collection;

/**
 * Created by astelmak on 26.05.16.
 */
public interface ComplexType extends Type {
    Collection<Field> getFields();

    Field getFieldByXSDName(String xsdName);
}
