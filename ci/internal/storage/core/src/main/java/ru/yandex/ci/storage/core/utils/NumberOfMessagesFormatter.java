package ru.yandex.ci.storage.core.utils;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class NumberOfMessagesFormatter {
    private NumberOfMessagesFormatter() {

    }

    public static String format(Map<? extends Enum<?>, ? extends List<?>> messagesByCase) {
        return messagesByCase.entrySet().stream()
                .map(x -> "%s - %d".formatted(x.getKey(), x.getValue().size()))
                .collect(Collectors.joining(", "));
    }
}
