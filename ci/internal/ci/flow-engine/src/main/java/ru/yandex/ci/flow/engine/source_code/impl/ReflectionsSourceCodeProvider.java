package ru.yandex.ci.flow.engine.source_code.impl;

import java.lang.reflect.Modifier;
import java.util.Collection;
import java.util.stream.Collectors;

import org.reflections.Reflections;

import ru.yandex.ci.core.common.SourceCodeEntity;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.source_code.SourceCodeProvider;

public class ReflectionsSourceCodeProvider implements SourceCodeProvider {
    public static final String SOURCE_CODE_PACKAGE = "ru.yandex.ci";

    private final String packageName;

    public ReflectionsSourceCodeProvider(String packageName) {
        this.packageName = packageName;
    }

    @Override
    public Collection<Class<? extends SourceCodeEntity>> load() {
        return new Reflections(this.packageName)
                .getSubTypesOf(SourceCodeEntity.class)
                .stream()
                .filter(c -> !Modifier.isAbstract(c.getModifiers()))
                /*
                 * загрузка классов типа SourceCodeEntity используется для создания
                 * мапы UUID -> SourceCodeObject, тип сущности к его описанию.
                 * Для получения UUID временно создается инстанс SourceCodeEntity для получения
                 * getSourceCodeId. При этом ProtobufResource может иметь разные UUID и описание каждого из типов
                 * создается динамически по протобафу. Поэтому этот конкретный класс исключен.
                 */
                // TODO создание инстанса для получения его UUID попахивает. Возможно, нужно перенести UUID в аннотацию.
                .filter(c -> c != Resource.class)
                .collect(Collectors.toList());
    }
}
