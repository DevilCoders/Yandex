package yandex.cloud.util;

import lombok.experimental.UtilityClass;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import static yandex.cloud.dashboard.util.ObjectUtils.firstNonNull;

/**
 * @author Vasiliy Briginets (0x40@yandex-team.ru)
 */
@UtilityClass
public class Strings {

    public static void forEach(String s, Pattern pattern, Consumer<Matcher> consumer) {
        Matcher matcher = pattern.matcher(s);
        while (matcher.find()) {
            consumer.accept(matcher);
        }
    }

//    public static String replaceMatch(String s, Pattern pattern, Function<String, String> replaceFunction) {
//        return replace(s, pattern, m -> replaceFunction.apply(m.group()));
//    }

    public static String replace(String s, Pattern pattern, Function<Matcher, String> replaceFunction) {
        Matcher matcher = pattern.matcher(s);
        if (!matcher.find()) {
            return s;
        }
        StringBuilder sb = new StringBuilder();
        do {
            matcher.appendReplacement(sb, Matcher.quoteReplacement(replaceFunction.apply(matcher)));
        } while (matcher.find());
        return matcher.appendTail(sb).toString();
    }

    public static String join(String delimiter, String... strings) {
        return Stream.of(strings)
                .filter(s -> !com.google.common.base.Strings.isNullOrEmpty(s))
                .collect(Collectors.joining(delimiter));
    }

    public static boolean isBlank(String s) {
        return s == null || s.trim().isEmpty();
    }

    /**
     * Splits a string, ignoring multiple spaces, but taking quotes into account.
     * E.g. "aa   'bb "   cc' \"dd ' ee\"" split will result in [aa, bb "   cc, dd ' ee].
     */
    public static List<String> quoteTokenize(String s) {
        List<String> result = new ArrayList<>();
        Pattern regex = Pattern.compile("\"([^\"]*)\"|'([^']*)'|([^\\s\"']+)");
        Matcher regexMatcher = regex.matcher(s);
        while (regexMatcher.find()) {
            result.add(firstNonNull(regexMatcher.group(1), regexMatcher.group(2), regexMatcher.group(3)));
        }
        return result;
    }

    /**
     * <p>Gets the substring before the first occurrence of a separator.
     * The separator is not returned.</p>
     *
     * <p> An empty ("") string input will return the empty string.</p>
     *
     * <p>If nothing is found, the string input is returned.</p>
     *
     * <pre>
     * StringUtils.substringBefore("", *)        = ""
     * StringUtils.substringBefore("abc", "")    = ""
     * StringUtils.substringBefore("abc", "a")   = ""
     * StringUtils.substringBefore("abcba", "b") = "a"
     * StringUtils.substringBefore("abc", "c")   = "ab"
     * StringUtils.substringBefore("abc", "d")   = "abc"
     * </pre>
     *
     * @param str       the String to get a substring from, may not be null
     * @param separator the String to search for, may not be null
     * @return the substring before the first occurrence of the separator
     */
    public static String substringBefore(String str, String separator) {
        int pos = str.indexOf(separator);
        if (pos == -1) {
            return str;
        }
        return str.substring(0, pos);
    }

    private static final Pattern UNDERSCORE_SUBSTRING = Pattern.compile("^(\\d*)(\\w)");

    /**
     * Like <code>CaseFormat.LOWER_UNDERSCORE.to(CaseFormat.UPPER_CAMEL, name)</code>,
     * but capitalizes letters not only after "_", but also after "_{digits}":
     * <code>ab_12_cd_34_56_ef_78gh</code> becomes <code>Ab12Cd3456Ef78Gh</code>
     * (note uppercase "G").
     * <p/>
     * See <a href="https://st.yandex-team.ru/CLOUD-29979">CLOUD-29979</a> for details.
     */
    public static String lowerUnderscoreToUpperCamel(String str) {
        return Arrays.stream(str.split("_"))
                .map(s -> Strings.replace(s, UNDERSCORE_SUBSTRING, m -> firstNonNull(m.group(1), "") + m.group(2).toUpperCase()))
                .collect(Collectors.joining());
    }

}
