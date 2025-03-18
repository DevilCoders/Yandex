package ru.yandex.ci.core.resolver;

import java.util.ArrayList;
import java.util.List;

import javax.annotation.Nonnull;

import lombok.Value;

public class JmesPathExtractor {

    static final char ESCAPE = '$';
    static final char PREFIX = '{';
    static final char SUFFIX = '}';

    static final String FULL_PREFIX = "" + ESCAPE + PREFIX;

    static final int ESTIMATE_PARTS = 4;

    private JmesPathExtractor() {
        //
    }

    /**
     * Splits provided string to separate part, each of it could (or could not) be replaced
     *
     * @return list of evaluation parts
     */
    static List<Part> extract(@Nonnull String string) {
        var result = new ArrayList<Part>(ESTIMATE_PARTS);

        var chars = string.toCharArray();
        var len = chars.length;
        var prefLen = FULL_PREFIX.length();

        // Splits the entire expression like "start ${a.key} ${b.value} $${escape} ${c} end" into separate replacement
        // each marked with attribute 'replace' (should be evaluated by JMESPath)

        // For this example it will be
        // 1) "start ":false
        // 2) "a.key": true
        // 3) " ": false
        // 4) "b.value": true
        // 5) " $": false
        // 6) "{escape}": false
        // 7) " ": false
        // 8) "c": true,
        // 9) " end": false


        int pos = 0;
        while (pos < len) {
            var start = string.indexOf(FULL_PREFIX, pos);
            if (start >= 0) {
                if (start > pos) {
                    var text = string.substring(pos, start);
                    result.add(new Part(text, false));
                }
                int end;
                int level = 1;
                for (end = start + prefLen; end < len; end++) {
                    var c = chars[end];
                    if (c == PREFIX) {
                        level++;
                    } else if (c == SUFFIX) {
                        level--;
                        if (level <= 0) {
                            break;
                        }
                    }
                }
                pos = end + 1;
                var endsWithSuffix = end < len && chars[end] == SUFFIX;
                if (start > 0 && chars[start - 1] == ESCAPE) {
                    var text = string.substring(start + 1, end + (endsWithSuffix ? 1 : 0));
                    result.add(new Part(text, false));
                } else {
                    var text = string.substring(start + (endsWithSuffix ? prefLen : 0), end);
                    result.add(new Part(text, endsWithSuffix));
                }
            } else {
                var text = string.substring(pos, len);
                result.add(new Part(text, false));
                break;
            }
        }
        return result;
    }

    @Value
    static class Part {
        String text;
        boolean replace;
    }
}
