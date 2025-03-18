package ru.yandex.ci.test.random;

import io.github.benas.randombeans.EnhancedRandomBuilder;
import io.github.benas.randombeans.api.EnhancedRandom;
import io.github.benas.randombeans.api.Randomizer;
import org.springframework.util.unit.DataSize;

public class DataSizeRandomizer implements Randomizer<DataSize> {

    private final EnhancedRandom random;

    public DataSizeRandomizer(long seed) {
        this.random = new EnhancedRandomBuilder().seed(seed).build();
    }

    public DataSizeRandomizer() {
        this.random = new EnhancedRandomBuilder().build();
    }

    @Override
    public DataSize getRandomValue() {
        return DataSize.ofBytes(random.nextInt(Integer.MAX_VALUE));
    }
}
