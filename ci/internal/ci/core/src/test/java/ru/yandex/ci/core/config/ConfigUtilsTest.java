package ru.yandex.ci.core.config;

import java.util.LinkedHashMap;
import java.util.Map;

import javax.annotation.Nullable;

import lombok.EqualsAndHashCode;
import lombok.ToString;
import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.lang.NonNullApi;

class ConfigUtilsTest {

    @Test
    void toMap() {
        var map = new LinkedHashMap<String, ConfigIdEntryImpl>();
        map.put("a", new ConfigIdEntryImpl(1));
        map.put("b", new ConfigIdEntryImpl(2));
        map.put("c", new ConfigIdEntryImpl(3));

        var list = ConfigUtils.toList(map);

        Assertions.assertThat(
                ConfigUtils.toMap(list)
        ).containsExactly(
                Map.entry("a", new ConfigIdEntryImpl("a", 1)),
                Map.entry("b", new ConfigIdEntryImpl("b", 2)),
                Map.entry("c", new ConfigIdEntryImpl("c", 3))
        );

        map.clear();
        map.put("c", new ConfigIdEntryImpl(3));
        map.put("b", new ConfigIdEntryImpl(2));
        map.put("a", new ConfigIdEntryImpl(1));
        list = ConfigUtils.toList(map);

        Assertions.assertThat(
                ConfigUtils.toMap(list)
        ).containsExactly(
                Map.entry("c", new ConfigIdEntryImpl("c", 3)),
                Map.entry("b", new ConfigIdEntryImpl("b", 2)),
                Map.entry("a", new ConfigIdEntryImpl("a", 1))
        );
    }


    @NonNullApi
    @ToString
    @EqualsAndHashCode
    private static class ConfigIdEntryImpl implements ConfigIdEntry<ConfigIdEntryImpl> {
        private String id;
        private final int value;

        private ConfigIdEntryImpl(int value) {
            this(null, value);
        }

        private ConfigIdEntryImpl(@Nullable String id, int value) {
            this.id = id;
            this.value = value;
        }

        @Override
        public String getId() {
            return id;
        }

        @Override
        public ConfigIdEntryImpl withId(String id) {
            this.id = id;
            return this;
        }
    }

}
