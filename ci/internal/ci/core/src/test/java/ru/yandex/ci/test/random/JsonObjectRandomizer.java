package ru.yandex.ci.test.random;

import com.google.gson.JsonObject;
import io.github.benas.randombeans.EnhancedRandomBuilder;
import io.github.benas.randombeans.api.EnhancedRandom;
import io.github.benas.randombeans.api.Randomizer;

public class JsonObjectRandomizer implements Randomizer<JsonObject> {

    private static final int MIN_FIELDS = 7;
    private static final int MAX_FIELDS = 21;
    private final EnhancedRandom random;

    public JsonObjectRandomizer(long seed) {
        this.random = new EnhancedRandomBuilder().seed(seed).build();
    }

    public JsonObjectRandomizer() {
        this.random = new EnhancedRandomBuilder().build();
    }

    @Override
    public JsonObject getRandomValue() {
        JsonObject object = new JsonObject();
        int elements = random.nextInt(MAX_FIELDS - MIN_FIELDS) + MIN_FIELDS;
        while (elements-- > 0) {
            String key = random.nextObject(String.class);
            switch (random.nextInt(3)) {
                case 0:
                    object.addProperty(key, random.nextObject(String.class));
                    break;
                case 1:
                    object.addProperty(key, random.nextInt());
                    break;
                case 2:
                    object.addProperty(key, random.nextDouble());
                    break;
                default:
                    throw new IllegalStateException();
            }
        }
        return object;

    }
}
