package ru.yandex.ci.tools.potato;

import java.lang.reflect.Type;
import java.net.http.HttpResponse;
import java.nio.charset.StandardCharsets;

import com.google.gson.FieldNamingPolicy;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

import ru.yandex.ci.util.gson.TypesafeType;

public class JsonBodyHandler<T> implements HttpResponse.BodyHandler<T> {

    private final Type type;
    private final Gson gson;

    private JsonBodyHandler(Class<T> clazz, Gson gson) {
        this.type = clazz;
        this.gson = gson;
    }

    private JsonBodyHandler(TypesafeType<T> type, Gson gson) {
        this.type = type;
        this.gson = gson;
    }

    public static <T> JsonBodyHandler<T> to(Class<T> clazz) {
        return new JsonBodyHandler<>(clazz, defaultGson());
    }

    public static <T> JsonBodyHandler<T> to(TypesafeType<T> type) {
        return new JsonBodyHandler<>(type, defaultGson());
    }

    public static <T> JsonBodyHandler<T> to(TypesafeType<T> type, Gson gson) {
        return new JsonBodyHandler<>(type, gson);
    }

    public static <T> JsonBodyHandler<T> to(Class<T> clazz, Gson gson) {
        return new JsonBodyHandler<>(clazz, gson);
    }

    private static Gson defaultGson() {
        return new GsonBuilder()
                .setFieldNamingPolicy(FieldNamingPolicy.LOWER_CASE_WITH_UNDERSCORES)
                .create();
    }

    @Override
    public HttpResponse.BodySubscriber<T> apply(HttpResponse.ResponseInfo responseInfo) {
        return asJSON(type);
    }

    private HttpResponse.BodySubscriber<T> asJSON(Type targetType) {
        HttpResponse.BodySubscriber<String> upstream = HttpResponse.BodySubscribers.ofString(StandardCharsets.UTF_8);

        return HttpResponse.BodySubscribers.mapping(
                upstream,
                (String body) -> parse(targetType, body));
    }

    protected T parse(Type targetType, String body) {
        return gson.fromJson(body, targetType);
    }
}
