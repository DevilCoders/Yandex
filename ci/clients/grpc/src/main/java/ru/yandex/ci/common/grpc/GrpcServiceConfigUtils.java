package ru.yandex.ci.common.grpc;

import java.io.IOException;
import java.io.UncheckedIOException;
import java.util.Map;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.core.io.Resource;

public class GrpcServiceConfigUtils {

    private static final ObjectMapper MAPPER = new ObjectMapper();

    private GrpcServiceConfigUtils() {
    }

    @SuppressWarnings("unchecked")
    static Map<String, ?> read(Resource resource) {
        try (var inputStream = resource.getInputStream()) {
            return MAPPER.readValue(inputStream, Map.class);
        } catch (IOException e) {
            throw new UncheckedIOException("cannot read grpc service config", e);
        }
    }
}
