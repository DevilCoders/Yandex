package ru.yandex.ci.test.random;

import java.util.function.Function;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonElement;

public class GsonPrettyFormatter<T extends JsonElement> implements Function<T, String> {
    private final Gson gson = new GsonBuilder().setPrettyPrinting().create();

    @Override
    public String apply(T element) {
        return gson.toJson(element);
    }
}
