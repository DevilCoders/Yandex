package ru.yandex.ci.util.gson;

import java.lang.reflect.Type;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

import com.google.gson.JsonDeserializationContext;
import com.google.gson.JsonDeserializer;
import com.google.gson.JsonElement;
import com.google.gson.JsonParseException;

public class LocalDateTimeAdapter implements JsonDeserializer<LocalDateTime> {

    private final DateTimeFormatter formatter;

    public LocalDateTimeAdapter(DateTimeFormatter formatter) {
        this.formatter = formatter;
    }

    public LocalDateTimeAdapter() {
        this(DateTimeFormatter.ISO_LOCAL_DATE_TIME);
    }

    @Override
    public LocalDateTime deserialize(JsonElement json, Type typeOfT,
                                     JsonDeserializationContext context) throws JsonParseException {
        return LocalDateTime.parse(json.getAsString(), formatter);
    }
}
