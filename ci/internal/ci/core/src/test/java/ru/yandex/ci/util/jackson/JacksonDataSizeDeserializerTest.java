package ru.yandex.ci.util.jackson;

import java.io.IOException;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.util.unit.DataSize;

import ru.yandex.ci.util.CiJson;

import static org.assertj.core.api.Assertions.assertThat;

@Slf4j
class JacksonDataSizeDeserializerTest {

    private ObjectMapper mapper;

    @BeforeEach
    public void setUp() {
        mapper = CiJson.mapper();
    }

    @Test
    void bytesDefault() throws IOException {
        assertThat(convert("17")).isEqualTo(DataSize.ofBytes(17));
    }

    @Test
    void bytesWithSuffix() throws IOException {
        assertThat(convert("17B")).isEqualTo(DataSize.ofBytes(17));
    }

    @Test
    void kiloBytesWithSuffix() throws IOException {
        assertThat(convert("17KB")).isEqualTo(DataSize.ofKilobytes(17));
    }

    @Test
    void kiloWithSuffix() throws IOException {
        assertThat(convert("17K")).isEqualTo(DataSize.ofKilobytes(17));
    }

    @Test
    void megaBytesWithSuffix() throws IOException {
        assertThat(convert("17MB")).isEqualTo(DataSize.ofMegabytes(17));
    }

    @Test
    void megaWithSuffix() throws IOException {
        assertThat(convert("17M")).isEqualTo(DataSize.ofMegabytes(17));
    }

    @Test
    void gigaBytesWithSuffix() throws IOException {
        assertThat(convert("17GB")).isEqualTo(DataSize.ofGigabytes(17));
        assertThat(convert("17Gb")).isEqualTo(DataSize.ofGigabytes(17));
    }

    @Test
    void gigaWithSuffix() throws IOException {
        assertThat(convert("17G")).isEqualTo(DataSize.ofGigabytes(17));
    }

    @Test
    void teraBytesWithSuffix() throws IOException {
        assertThat(convert("17TB")).isEqualTo(DataSize.ofTerabytes(17));
    }

    @Test
    void teraWithSuffix() throws IOException {
        assertThat(convert("17T")).isEqualTo(DataSize.ofTerabytes(17));
    }

    @Test
    void decimal() throws IOException {
        assertThat(convert("17.6")).isEqualTo(DataSize.ofBytes(18));
    }

    @Test
    void withSpace() throws IOException {
        assertThat(convert("45 KB")).isEqualTo(DataSize.ofKilobytes(45));
    }

    @Test
    void withoutSpace() throws IOException {
        assertThat(convert("17GB")).isEqualTo(DataSize.ofGigabytes(17));
    }

    @Test
    void decimalWithScale() throws IOException {
        assertThat(convert("1.5 GB")).isEqualTo(DataSize.ofBytes(1610612736));
    }

    @Test
    void caseInsensitive() throws IOException {
        assertThat(convert("17gb")).isEqualTo(DataSize.ofGigabytes(17));
        assertThat(convert("17 mb")).isEqualTo(DataSize.ofMegabytes(17));
    }

    @Test
    void testSerializeDeserialize() throws IOException {
        var expect = new DataSizeClass(DataSize.ofKilobytes(1024), 44);
        var json = mapper.writer().writeValueAsString(expect);


        log.info("JSON: {}", json);
        var actual = mapper.reader().readValue(json, DataSizeClass.class);
        assertThat(expect)
                .isEqualTo(actual);
    }

    private DataSize convert(String text) throws IOException {
        var rec = mapper.reader().readValue("""
                {"size": "%s", "value": 44}
                """.formatted(text), DataSizeClass.class);
        assertThat(rec.getValue()).isEqualTo(44);
        return rec.getSize();
    }

    @Value
    static class DataSizeClass {
        @JsonDeserialize(using = JacksonDataSizeDeserializer.class)
        DataSize size;
        int value;
    }
}
