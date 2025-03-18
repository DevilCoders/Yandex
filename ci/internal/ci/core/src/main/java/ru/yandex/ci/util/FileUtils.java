package ru.yandex.ci.util;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.google.common.base.Preconditions;
import org.springframework.util.unit.DataSize;

public class FileUtils {
    private static final Pattern SIZE_PATTERN = Pattern.compile("([\\d.,]+)\\s*(\\w)?");

    private FileUtils() {
    }

    public static DataSize parseFileSize(String text) {
        final Matcher m = SIZE_PATTERN.matcher(text.trim().replaceAll(",", "."));
        Preconditions.checkState(m.find());
        String suffix = m.group(2);

        DataSize size;
        if (suffix != null) {
            size = switch (suffix.toUpperCase().charAt(0)) {
                case 'T' -> DataSize.ofTerabytes(1);
                case 'G' -> DataSize.ofGigabytes(1);
                case 'M' -> DataSize.ofMegabytes(1);
                case 'K' -> DataSize.ofKilobytes(1);
                case 'B' -> DataSize.ofBytes(1);
                default -> throw new IllegalArgumentException("Unsupported size: " + suffix);
            };
        } else {
            size = DataSize.ofBytes(1);
        }
        return DataSize.ofBytes(Math.round(Double.parseDouble(m.group(1)) * size.toBytes()));
    }

    public static <T> T parseJson(String resource, Class<T> target) {
        try {
            return CiJson.mapper().readValue(ResourceUtils.textResource(resource), target);
        } catch (JsonProcessingException e) {
            throw new RuntimeException("Unable to parse resource " + resource, e);
        }
    }

    public static <T> T parseJson(String resource, TypeReference<T> target) {
        try {
            return CiJson.mapper().readValue(ResourceUtils.textResource(resource), target);
        } catch (JsonProcessingException e) {
            throw new RuntimeException("Unable to parse resource " + resource, e);
        }
    }
}
