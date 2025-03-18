package ru.yandex.ci.storage.core.db.model.test_diff;

import java.util.Collection;
import java.util.List;
import java.util.stream.Stream;

public class TagUtils {
    private TagUtils() {

    }

    public static String toTagsString(Collection<String> tags) {
        return tags.isEmpty() ? "" : "|" + String.join("|", tags) + "|";
    }

    public static List<String> fromTagsString(String tags) {
        return Stream.of(tags.split("\\|"))
                .filter(s -> !s.isEmpty())
                .toList();
    }
}
