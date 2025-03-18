package ru.yandex.ci.storage.core.db.model.test_diff;

import java.util.List;

import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

class TagUtilsTest {

    @Test
    void testConvertEmpty() {
        var tags = TagUtils.toTagsString(List.of());
        assertThat(tags)
                .isEqualTo("");
        assertThat(TagUtils.fromTagsString(tags))
                .isEqualTo(List.of());
    }

    @Test
    void testConvertNotEmpty() {
        var tags = TagUtils.toTagsString(List.of("a", "b", "c"));
        assertThat(tags)
                .isEqualTo("|a|b|c|");
        assertThat(TagUtils.fromTagsString(tags))
                .isEqualTo(List.of("a", "b", "c"));
    }
}
