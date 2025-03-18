package ru.yandex.ci.util;

import java.nio.file.Path;

import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

class GlobMatchersTest {

    @Test
    void escapeMetaChars() {
        var path = "A\\*[{}]Z";
        var escaped = GlobMatchers.escapeMetaChars(path);
        var globMatcher = GlobMatchers.getGlobMatcher(escaped);
        assertThat(globMatcher.matches(Path.of(path))).isTrue();
    }
}
