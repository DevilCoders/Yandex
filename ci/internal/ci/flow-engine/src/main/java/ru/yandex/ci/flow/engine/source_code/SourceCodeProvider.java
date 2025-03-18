package ru.yandex.ci.flow.engine.source_code;


import java.util.Collection;

import ru.yandex.ci.core.common.SourceCodeEntity;

public interface SourceCodeProvider {
    Collection<Class<? extends SourceCodeEntity>> load();
}
