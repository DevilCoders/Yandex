package ru.yandex.ci.storage.core.converter;

import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

public class TskvConverter {
    public String convertLine(Map<String, String> data) {
        var builder = new StringBuilder();
        convertLineInternal(builder, data);

        return builder.toString();
    }

    public String convertLines(List<? extends Map<String, String>> data) {
        var builder = new StringBuilder();
        for (var line : data) {
            convertLineInternal(builder, line);
        }

        return builder.toString();
    }

    private void convertLineInternal(StringBuilder builder, Map<String, String> data) {
        for (var e : data.entrySet()) {
            builder.append("\t")
                    .append(e.getKey())
                    .append("=");
            appendValueWithEscaping(builder, e.getValue());
        }

        builder.append("\n");
    }

    private void appendValueWithEscaping(StringBuilder builder, @Nullable String value) {
        if (value == null) {
            builder.append("null");
            return;
        }

        for (int i = 0; i < value.length(); ++i) {
            char c = value.charAt(i);
            switch (c) {
                case '\t' -> builder.append("\\t");
                case '\n' -> builder.append("\\n");
                case '\r' -> builder.append("\\r");
                case '\0' -> builder.append("\\0");
                case '\\' -> builder.append("\\\\");
                case '"' -> builder.append("\\\"");
                default -> builder.append(c);
            }
        }
    }
}
