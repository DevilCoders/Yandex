package ru.yandex.ci.test;

import org.junit.jupiter.api.extension.Extension;
import org.slf4j.bridge.SLF4JBridgeHandler;

/**
 * Класс для установки моста между java.util.logging и slf4j.
 * Некоторые тесты зависят от сервисов, которые использую jul, что приводит к выводу всей информации в stderr.
 * Из-за чего ya test пугается, и плюет сообщениями в лог. Данный класс заворачивает логи в slf4j который уже
 * можно настраивать по желанию. Как правило, через log4j2-test.yaml
 */
public final class JULBridgeExtension implements Extension {
    static {
        if (!SLF4JBridgeHandler.isInstalled()) {
            SLF4JBridgeHandler.removeHandlersForRootLogger();
            SLF4JBridgeHandler.install();
        }
    }
}
