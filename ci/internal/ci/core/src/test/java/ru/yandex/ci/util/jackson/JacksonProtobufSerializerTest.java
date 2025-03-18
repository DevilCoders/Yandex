package ru.yandex.ci.util.jackson;

import java.time.LocalDate;
import java.time.Month;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.Value;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.test.schema.Couple;
import ru.yandex.ci.core.test.schema.Person;
import ru.yandex.ci.util.CiJson;

import static org.assertj.core.api.Assertions.assertThat;

class JacksonProtobufSerializerTest {
    private static final String JSON = """
            {"marriageDate":"2000-11-18",\
            "couple":{"he":{"age":77,"first_name":"Michael"},"she":{"age":52,"first_name":"Catherine"}}}""";

    private static final MarriageCertificate OBJECT = new MarriageCertificate(
            LocalDate.of(2000, Month.NOVEMBER, 18),
            Couple.newBuilder()
                    .setHe(Person.newBuilder()
                            .setFirstName("Michael")
                            .setAge(77)
                            .build())
                    .setShe(Person.newBuilder()
                            .setFirstName("Catherine")
                            .setAge(52)
                            .build())
                    .build());
    private ObjectMapper mapper;

    @BeforeEach
    public void setUp() {
        mapper = CiJson.mapper();
    }

    @Test
    void serialize() throws JsonProcessingException {
        assertThat(mapper.writeValueAsString(OBJECT)).isEqualTo(JSON);
    }

    @Test
    void deserialize() throws JsonProcessingException {
        assertThat(mapper.readValue(JSON, MarriageCertificate.class)).isEqualTo(OBJECT);
    }

    @Value
    static class MarriageCertificate {
        LocalDate marriageDate;

        @JsonSerialize(using = JacksonProtobufSerializer.class)
        @JsonDeserialize(using = JacksonProtobufDeserializer.class)
        Couple couple;
    }
}
