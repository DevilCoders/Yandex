package ru.yandex.ci.util.jackson.parse;

import java.util.List;
import java.util.Map;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.PropertyNamingStrategies;
import com.fasterxml.jackson.dataformat.yaml.YAMLFactory;
import com.fasterxml.jackson.dataformat.yaml.YAMLGenerator;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.Getter;
import lombok.NoArgsConstructor;
import lombok.Setter;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.test.TestUtils.textResource;

class ParseInfoModuleTest {

    private ObjectMapper mapper;

    @BeforeEach
    void setUp() {
        YAMLFactory yamlFactory = new YAMLFactory()
                .disable(YAMLGenerator.Feature.WRITE_DOC_START_MARKER)
                .enable(YAMLGenerator.Feature.MINIMIZE_QUOTES);

        mapper = new ObjectMapper(yamlFactory)
                .registerModule(new ParseInfoModule())
                .setPropertyNamingStrategy(PropertyNamingStrategies.SNAKE_CASE);
    }

    @Test
    void root() throws JsonProcessingException {
        Person parsed = mapper.readValue("first_name: Billy", Person.class);
        assertThat(parsed.getName()).isEqualTo("Billy");
        assertThat(parsed.getParseInfo().getParsedPath("first_name")).isEqualTo("/first_name");
    }

    @Test
    void nested() throws JsonProcessingException {
        Person node = mapper.readValue(textResource("parse-info/complex.yaml"), Person.class);
        Person arthur = node.getBelovedChildren().get(0).getBelovedChildren().get(1);

        assertThat(arthur.getName()).isEqualTo("Arthur");
        assertThat(arthur.getParseInfo().getParsedPath())
                .isEqualTo("/beloved_children/0/beloved_children/1");
    }

    @Test
    void nestedMap() throws JsonProcessingException {
        Person node = mapper.readValue(textResource("parse-info/complex.yaml"), Person.class);
        Person tommy = node.getBelovedChildren().get(0).getRelatives().get("aunt").getBelovedChildren().get(0);

        assertThat(tommy.getName()).isEqualTo("Tommy");
        assertThat(tommy.getParseInfo().getParsedPath("first_name"))
                .isEqualTo("/beloved_children/0/relatives/aunt/beloved_children/0/first_name");
    }

    @Test
    void setterTag() throws JsonProcessingException {
        GetterTag parsed = mapper.readValue("custom_field_name: inner-value", GetterTag.class);
        assertThat(parsed.getValue()).isEqualTo("inner-value");
        assertThat(parsed.getParseInfo().getParsedPath("custom_field_name")).isEqualTo("/custom_field_name");
    }

    @Data
    @NoArgsConstructor
    @AllArgsConstructor
    public static class Person implements HasParseInfo {

        List<Person> belovedChildren;

        @JsonProperty("first_name")
        String name;

        Map<String, Person> relatives;

        @Getter(onMethod_ = @Override)
        @Setter(onMethod_ = @Override)
        ParseInfo parseInfo;
    }

    public static class GetterTag implements HasParseInfo {

        @Getter(onMethod_ = @Override)
        @Setter(onMethod_ = @Override)
        private ParseInfo parseInfo;

        private String value;

        @JsonProperty("custom_field_name")
        public String getValue() {
            return value;
        }

        @JsonProperty("custom_field_name")
        public void setValue(String value) {
            this.value = value;
        }

    }
}
