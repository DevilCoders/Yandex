package ru.yandex.ci.util.jackson.parse;

import com.fasterxml.jackson.databind.module.SimpleModule;

/**
 * Добавляет к классам, которые реализуют {@link HasParseInfo}, значения в {@link ParseInfo}.
 *
 * Позволяет получать информацию о том, по какому пути в json/yaml было получено значение при
 * парсинге. Можно использовать для предоставления более подробной информации в ошибках валидации.
 */
public class ParseInfoModule extends SimpleModule {
    public ParseInfoModule() {
        setDeserializerModifier(new ParseInfoProviderRegistrar());
    }
}
