package yandex.cloud.util;

import com.google.common.collect.ImmutableMap;
import org.junit.Assert;
import org.junit.Test;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;

import static java.util.Arrays.asList;
import static org.assertj.core.api.Assertions.assertThat;

/**
 * @author ssytnik
 */
public class StringsTest {
    @Test
    public void replaceMulti() {
        int[] i = {1};
        assertThat(Strings.replace("xxx", Pattern.compile("x"), m -> String.valueOf(i[0]++))).isEqualTo("123");
    }

    @Test
    public void replaceWithDollars() {
        assertThat(Strings.replace("xxx", Pattern.compile("x"), m -> "$")).isEqualTo("$$$");
    }

    @Test
    public void join() {
        Assert.assertEquals("url/path", Strings.join("?", "url/path", null));
        Assert.assertEquals("url/path", Strings.join("?", "url/path", ""));
        Assert.assertEquals("url/path?a=1&b=2", Strings.join("?", "url/path", "a=1&b=2"));
    }

    @Test
    public void forEach() {
        Map<String, String> map = new HashMap<>();

        Strings.forEach("a='b', c='d', ee='ff'", Pattern.compile("(\\w+)='(\\w+)'"), m -> {
            map.put(m.group(1), m.group(2));
        });

        Assert.assertEquals(ImmutableMap.of("a", "b", "c", "d", "ee", "ff"), map);
    }

    @Test
    public void quoteTokenize() {
        Assert.assertEquals(List.of(), Strings.quoteTokenize(""));

        Assert.assertEquals(asList("a", "bb", "ccc", "dd \" dd", "ee ' ff", "gg"),
                Strings.quoteTokenize("  a bb  ccc 'dd \" dd' \"ee ' ff\" gg   "));

        Assert.assertEquals(List.of("file.yaml", "", ""), Strings.quoteTokenize("file.yaml '' \"\""));
    }

    @Test
    public void substringBefore() {
        Assert.assertEquals("", Strings.substringBefore("", "a"));
        Assert.assertEquals("", Strings.substringBefore("abc", ""));
        Assert.assertEquals("", Strings.substringBefore("abc", "a"));
        Assert.assertEquals("a", Strings.substringBefore("abcba", "b"));
        Assert.assertEquals("ab", Strings.substringBefore("abc", "c"));
        Assert.assertEquals("abc", Strings.substringBefore("abc", "d"));
    }

    @Test
    public void lowerUnderscoreToUpperCamel() {
        Assert.assertEquals("", Strings.lowerUnderscoreToUpperCamel(""));
        Assert.assertEquals("Abc", Strings.lowerUnderscoreToUpperCamel("abc"));
        Assert.assertEquals("AbcDef", Strings.lowerUnderscoreToUpperCamel("abc_def"));
        Assert.assertEquals("123", Strings.lowerUnderscoreToUpperCamel("123"));
        Assert.assertEquals("Ab12Cd3456Ef78Gh", Strings.lowerUnderscoreToUpperCamel("ab_12_cd_34_56_ef_78gh"));
        Assert.assertEquals("PostgresqlConfig96", Strings.lowerUnderscoreToUpperCamel("postgresql_config_9_6"));
        Assert.assertEquals("PostgresqlConfig101C", Strings.lowerUnderscoreToUpperCamel("postgresql_config_10_1c"));
    }
}
