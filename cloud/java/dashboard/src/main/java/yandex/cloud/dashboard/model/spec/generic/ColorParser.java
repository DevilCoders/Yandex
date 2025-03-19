package yandex.cloud.dashboard.model.spec.generic;

import com.google.common.base.Strings;
import lombok.AllArgsConstructor;

import java.util.List;
import java.util.PrimitiveIterator.OfInt;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.IntStream;

import static com.google.common.base.Preconditions.checkArgument;
import static java.lang.Double.parseDouble;
import static java.lang.Integer.parseInt;

/**
 * @author girevoyt
 * @author ssytnik
 */
@AllArgsConstructor
class ColorParser {
    private static final List<Integer> LENGTHS = List.of(4, 5, 7, 9);
    private static final Pattern PATTERN =
            Pattern.compile("rgb(a)?\\s?\\(\\s?(\\d+),\\s?(\\d+)\\s?,\\s?(\\d+)\\s?(,\\s?([\\d.]+))?\\)");

    int r;
    int g;
    int b;
    Double a;

    private static ColorParser parse(String s) {
        checkArgument(!Strings.isNullOrEmpty(s), "Unexpected RGB(A) value: '%s'", s);
        if (s.charAt(0) == '#') {
            checkArgument(LENGTHS.contains(s.length()), "Unexpected RGB(A) length: '%s'", s);
            int l = s.length() <= 5 ? 1 : 2;
            int n = (s.length() - 1) / l;
            OfInt it = IntStream.range(0, n)
                    .map(i -> parseInt(s.substring(1 + i * l, 1 + (i + 1) * l), 16) * (l == 1 ? 0x11 : 0x1))
                    .iterator();
            return new ColorParser(it.next(), it.next(), it.next(), it.hasNext() ? it.next() / 255.0 : null);
        } else {
            Matcher m = PATTERN.matcher(s);
            checkArgument(m.matches(), "Unexpected RGB(A) value: '%s'", s);
            boolean hasA = "a".equals(m.group(1));
            checkArgument(hasA == (m.group(6) != null), "Alpha channel%s found", hasA ? " not" : "");
            return new ColorParser(parseInt(m.group(2)), parseInt(m.group(3)), parseInt(m.group(4)),
                    hasA ? parseDouble(m.group(6)) : null);
        }
    }

    static ColorSpec parseColor(String rgb) {
        ColorParser cp = parse(rgb);
        checkArgument(cp.a == null, "Color cannot have alpha channel: '%s'", rgb);
        return new ColorSpec(cp.r, cp.g, cp.b);
    }

    static RGBASpec parseRGBA(String rgba) {
        ColorParser cp = parse(rgba);
        checkArgument(cp.a != null, "RGBA should have alpha channel: '%s'", rgba);
        return new RGBASpec(new ColorSpec(cp.r, cp.g, cp.b), cp.a);
    }
}
