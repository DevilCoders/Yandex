package ru.yandex.ci.flow.engine.source_code.model;

import ru.yandex.ci.core.common.SourceCodeEntity;

public class ExecutorOwnerExtractor {
    public static final String ROOT_PACKAGE = "ru.yandex.ci.flows.";
    public static final String MARKET_INFRA = "market_infra";

    private ExecutorOwnerExtractor() {

    }

    public static String extract(Class<? extends SourceCodeEntity> clazz) {
        String packageName = clazz.getPackage().getName();
        if (!packageName.startsWith(ROOT_PACKAGE)) {
            return packageName.startsWith("ru.yandex.ci") ? MARKET_INFRA : "";
        }

        String childPackage = packageName.substring(ROOT_PACKAGE.length());

        return childPackage.split("\\.", 2)[0];
    }
}
