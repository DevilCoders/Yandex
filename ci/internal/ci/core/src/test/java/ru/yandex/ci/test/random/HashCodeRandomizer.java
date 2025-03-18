package ru.yandex.ci.test.random;

import com.google.common.hash.HashCode;
import io.github.benas.randombeans.EnhancedRandomBuilder;
import io.github.benas.randombeans.api.EnhancedRandom;
import io.github.benas.randombeans.api.Randomizer;

public class HashCodeRandomizer implements Randomizer<HashCode> {

    private final EnhancedRandom random;

    public HashCodeRandomizer(long seed) {
        this.random = new EnhancedRandomBuilder().seed(seed).build();
    }

    public HashCodeRandomizer() {
        this.random = new EnhancedRandomBuilder().build();
    }

    @Override
    public HashCode getRandomValue() {
        byte[] bytes = new byte[20];
        random.nextBytes(bytes);
        return HashCode.fromBytes(bytes);
    }
}
