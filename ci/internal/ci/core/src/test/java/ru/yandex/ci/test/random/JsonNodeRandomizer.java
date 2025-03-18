package ru.yandex.ci.test.random;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.DoubleNode;
import com.fasterxml.jackson.databind.node.IntNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.fasterxml.jackson.databind.node.TextNode;
import io.github.benas.randombeans.EnhancedRandomBuilder;
import io.github.benas.randombeans.api.EnhancedRandom;
import io.github.benas.randombeans.api.Randomizer;

public class JsonNodeRandomizer implements Randomizer<JsonNode> {

    private static final int MIN_FIELDS = 2;
    private static final int MAX_FIELDS = 5;
    private static final int DEPTH = 2;
    private final ObjectMapper mapper = new ObjectMapper();
    private final EnhancedRandom random;

    public JsonNodeRandomizer(long seed) {
        this.random = new EnhancedRandomBuilder().seed(seed).build();
    }

    public JsonNodeRandomizer() {
        this.random = new EnhancedRandomBuilder().build();
    }

    @Override
    public JsonNode getRandomValue() {
        return getRandomValue(DEPTH);
    }

    private JsonNode getRandomValue(int depth) {
        if (depth == 0) {
            switch (random.nextInt(3)) {
                case 0:
                    return TextNode.valueOf(random.nextObject(String.class));
                case 1:
                    return IntNode.valueOf(random.nextInt());
                case 2:
                default:
                    return DoubleNode.valueOf(random.nextDouble());
            }
        }

        boolean createArray = random.nextBoolean();
        int elements = random.nextInt(MAX_FIELDS - MIN_FIELDS) + MIN_FIELDS;
        if (createArray) {
            ArrayNode arrayNode = mapper.createArrayNode();
            while (elements-- > 0) {
                arrayNode.add(getRandomValue(depth - 1));
            }
            return arrayNode;
        } else {
            ObjectNode objectNode = mapper.createObjectNode();
            while (elements-- > 0) {
                String key = random.nextObject(String.class);
                objectNode.set(key, getRandomValue(depth - 1));
            }
            return objectNode;
        }
    }

}
