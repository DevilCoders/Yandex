package ru.yandex.ci.storage.core.util;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.utils.HtmlSnippetFormatter;

import static org.assertj.core.api.Assertions.assertThat;

public class HtmlSnippetFormatterTest {

    @Test
    public void testFormatNoTag() {
        assertThat(HtmlSnippetFormatter.format("[[test]]"))
                .isEqualTo("[[test]]");
    }

    @Test
    public void testFormatColorRed() {
        assertThat(HtmlSnippetFormatter.format("[[test]][[c:red]][[rst]]"))
                .isEqualTo("[[test]]<span class='hljs-snippet-red'></span>");
    }

    @Test
    public void testFormatColorNotInColorsBlack() {
        assertThat(HtmlSnippetFormatter.format("[[a]][[c:black]][[bad]]"))
                .isEqualTo("[[a]]");
    }

    @Test
    public void testFormatPath() {
        assertThat(HtmlSnippetFormatter.format("[[a]][[path]][[test]]"))
                .isEqualTo("[[a]]<span class='hljs-snippet-path'>[[test]]</span>");
    }

    @Test
    public void testQuoteUrlNotNull() {
        assertThat(HtmlSnippetFormatter.format("[[path]]test"))
                .isEqualTo("<a href='test' target='_blank' class='hljs-snippet-path'>test</a>");
    }

    @Test
    public void testDecorateColorNotNull() {
        assertThat(HtmlSnippetFormatter.format("[[imp]]test"))
                .isEqualTo("<span class='hljs-snippet-imp'>test</span>");
    }
}
