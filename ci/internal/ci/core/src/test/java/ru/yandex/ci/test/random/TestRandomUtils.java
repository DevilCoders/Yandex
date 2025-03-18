package ru.yandex.ci.test.random;

import java.nio.file.Path;
import java.time.Duration;
import java.util.Random;
import java.util.UUID;

import com.fasterxml.jackson.databind.JsonNode;
import com.google.common.base.Suppliers;
import com.google.common.hash.HashCode;
import com.google.gson.JsonObject;
import io.github.benas.randombeans.EnhancedRandomBuilder;
import io.github.benas.randombeans.api.EnhancedRandom;
import io.github.benas.randombeans.randomizers.misc.EnumRandomizer;
import org.springframework.util.unit.DataSize;

import ru.yandex.ci.api.proto.Common;

public class TestRandomUtils {
    private TestRandomUtils() {
    }

    public static byte[] bytes(Random random) {
        byte[] b = new byte[256];
        random.nextBytes(b);
        return b;
    }

    public static String string(Random random, String prefix) {
        return prefix + "-" + random.nextInt(1000);
    }

    public static UUID fastUUID(Random random) {
        return new UUID(random.nextLong(), random.nextLong());
    }

    public static EnhancedRandom enhancedRandom(long seed) {
        return enhancedRandomBuilder(seed)
                .build();
    }

    public static EnhancedRandomBuilder enhancedRandomBuilder(long seed) {
        return new EnhancedRandomBuilder()
                .seed(seed)
                .overrideDefaultInitialization(true)
                .randomize(Path.class, Suppliers.memoize(() -> Path.of("some/dir")))
                .randomize(JsonNode.class, new JsonNodeRandomizer(seed))
                .randomize(JsonObject.class, new JsonObjectRandomizer(seed))
                .randomize(HashCode.class, new HashCodeRandomizer(seed))
                .randomize(Common.FlowType.class,
                        new EnumRandomizer<>(Common.FlowType.class, Common.FlowType.UNRECOGNIZED))
                .randomize(DataSize.class, new DataSizeRandomizer(seed))
                .randomize(Duration.class, new DurationRandomizer(seed));
    }
}
