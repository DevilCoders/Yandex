package ru.yandex.ci.util;

import java.util.List;

import com.google.common.base.CharMatcher;
import com.google.common.base.Splitter;

public class SpringParseUtils {

    private static final Splitter SPLITTER = Splitter.on(",")
            .omitEmptyStrings()
            .trimResults(CharMatcher.whitespace());

    private SpringParseUtils() {
        //
    }

    public static double[] parseToDoubleArray(String value) {
        return parseToStringList(value).stream()
                .mapToDouble(Double::valueOf)
                .toArray();
    }

    public static List<String> parseToStringList(String property) {
        return SPLITTER.splitToList(property);
    }
}
