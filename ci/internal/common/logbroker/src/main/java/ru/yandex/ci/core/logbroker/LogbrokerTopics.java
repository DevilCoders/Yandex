package ru.yandex.ci.core.logbroker;

import java.util.List;

import com.google.common.base.CharMatcher;
import com.google.common.base.Splitter;

public class LogbrokerTopics {
    private final List<String> value;

    private LogbrokerTopics(List<String> value) {
        this.value = value;
    }

    public List<String> get() {
        return value;
    }

    public static LogbrokerTopics parse(String topics) {
        var splitter = Splitter.on(",")
                .omitEmptyStrings()
                .trimResults(CharMatcher.whitespace());

        return new LogbrokerTopics(splitter.splitToList(topics));
    }
}
