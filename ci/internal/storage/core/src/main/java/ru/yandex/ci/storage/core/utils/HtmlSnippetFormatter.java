package ru.yandex.ci.storage.core.utils;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.Set;
import java.util.regex.Pattern;

import javax.annotation.Nullable;

import org.springframework.web.util.HtmlUtils;

public class HtmlSnippetFormatter {
    public static final Set<String> KNOWN_COLORS = Set.of(
            "blue",
            "cyan",
            "default",
            "green",
            "grey",
            "magenta",
            "red",
            "white",
            "yellow",
            "light-blue",
            "light-cyan",
            "light-default",
            "light-green",
            "light-grey",
            "light-magenta",
            "light-red",
            "light-white",
            "light-yellow"
    );

    public static final Set<String> KNOWN_TAGS =
            Set.of(
                    "rst",
                    "path",
                    "imp",
                    "unimp",
                    "bad",
                    "warn",
                    "good",
                    "alt1",
                    "alt2",
                    "alt3"
            );

    private static final String CLASS_PREFIX = "hljs-snippet-";

    private static final Pattern MARKUP_RE = Pattern.compile("\\[\\[([^\\[\\]]*?)\\]\\]");

    private HtmlSnippetFormatter() {

    }

    private static void transform(String marker, String text, StringBuilder result) {
        if (marker.startsWith("c:")) {
            var color = marker.substring("c:".length());
            if (!KNOWN_COLORS.contains(color)) {
                result.append(text);
            } else {
                colorize(color, text, result);
            }
        } else {
            decorate(marker, text, result);
        }
    }

    private static boolean isTag(String tag) {
        if (tag.startsWith("c:")) {
            return true;
        }

        return KNOWN_TAGS.contains(tag);
    }

    public static String format(@Nullable String text) {
        if (text == null || text.isEmpty()) {
            return "";
        }

        var result = new StringBuilder();

        var sb = new StringBuilder();
        String marker = "";
        int start = 0;

        var m = MARKUP_RE.matcher(text);
        while (m.find()) {
            String tag = m.group(1);
            if (!isTag(tag)) {
                continue;
            }

            if (start != m.start()) {
                sb.append(text, start, m.start());
            }

            transform(marker, sb.toString(), result);

            marker = tag;
            sb.setLength(0);

            start = m.end();
        }

        if (start != text.length()) {
            sb.append(text.substring(start));
            transform(marker, sb.toString(), result);
        }

        return result.toString();
    }

    private static String escape(String s) {
        return HtmlUtils.htmlEscape(s);
    }

    private static String quoteUrl(String s) {
        try {
            return new URI(s).toASCIIString();
        } catch (URISyntaxException e) {
            return "";
        }
    }

    private static void decorate(String marker, String text, StringBuilder result) {
        var tag = KNOWN_TAGS.contains(marker) ? marker : null;

        if ("path".equals(marker)) {
            var url = quoteUrl(text);
            if (url.isEmpty()) {
                colorize(tag, text, result);
            } else {
                result.append("<a href='");
                result.append(url);
                result.append("' target='_blank' class='");
                result.append(CLASS_PREFIX);
                result.append(tag);
                result.append("'>");
                result.append(escape(text));
                result.append("</a>");
            }
        } else if (tag != null) {
            colorize(tag, text, result);
        } else {
            result.append(escape(text));
        }
    }

    private static void colorize(String tag, String text, StringBuilder result) {
        result.append("<span class='");
        result.append(CLASS_PREFIX);
        result.append(tag);
        result.append("'>");
        result.append(escape(text));
        result.append("</span>");
    }
}
