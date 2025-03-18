package ru.yandex.ci.util.gson;

import java.lang.reflect.Type;
import java.time.LocalDate;
import java.time.format.DateTimeFormatter;

import com.google.gson.JsonDeserializationContext;
import com.google.gson.JsonDeserializer;
import com.google.gson.JsonElement;
import com.google.gson.JsonParseException;

public class LocalDateAdapter implements JsonDeserializer<LocalDate> {

    private final DateTimeFormatter formatter;

    public LocalDateAdapter(DateTimeFormatter formatter) {
        this.formatter = formatter;
    }

    public LocalDateAdapter() {
        this(DateTimeFormatter.ISO_LOCAL_DATE_TIME);
    }

    @Override
    public LocalDate deserialize(JsonElement json, Type typeOfT,
                                 JsonDeserializationContext context) throws JsonParseException {
        return LocalDate.parse(json.getAsString(), formatter);
    }
}

