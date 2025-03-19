package yandex.cloud.dashboard.util;

import com.fasterxml.jackson.annotation.JsonInclude.Include;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.SerializationFeature;
import lombok.SneakyThrows;
import lombok.experimental.UtilityClass;

import static com.fasterxml.jackson.annotation.JsonAutoDetect.Visibility.ANY;
import static com.fasterxml.jackson.annotation.JsonAutoDetect.Visibility.NONE;

@UtilityClass
public class Json {
    private static final ObjectMapper mapper = new ObjectMapper() {{
        configure(SerializationFeature.INDENT_OUTPUT, true);
        setSerializationInclusion(Include.NON_NULL);
        setVisibility(getSerializationConfig().getDefaultVisibilityChecker()
                .withFieldVisibility(ANY)
                .withGetterVisibility(NONE)
                .withIsGetterVisibility(NONE)
                .withSetterVisibility(NONE)
        );
    }};

    @SneakyThrows
    public static String toJson(Object o) {
        return mapper.writeValueAsString(o);
    }

    @SneakyThrows
    public static <T> T fromJson(Class<T> clazz, String content) {
        return mapper.readerFor(mapper.getTypeFactory().constructType(clazz)).readValue(content);
    }

}
