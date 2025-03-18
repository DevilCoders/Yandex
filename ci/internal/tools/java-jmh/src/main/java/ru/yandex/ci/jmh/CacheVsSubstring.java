package ru.yandex.ci.jmh;

import java.util.concurrent.ExecutionException;
import java.util.stream.IntStream;

import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import org.apache.commons.lang3.RandomStringUtils;
import org.apache.commons.lang3.RandomUtils;
import org.openjdk.jmh.annotations.Benchmark;
import org.openjdk.jmh.annotations.BenchmarkMode;
import org.openjdk.jmh.annotations.Measurement;
import org.openjdk.jmh.annotations.Mode;
import org.openjdk.jmh.annotations.Scope;
import org.openjdk.jmh.annotations.State;
import org.openjdk.jmh.annotations.Warmup;
import org.openjdk.jmh.runner.Runner;
import org.openjdk.jmh.runner.RunnerException;
import org.openjdk.jmh.runner.options.Options;
import org.openjdk.jmh.runner.options.OptionsBuilder;

/*
Benchmark                          Mode  Cnt         Score         Error  Units
CacheVsSubstring.shortNameCached  thrpt    3   5490376.695 ± 1531856.610  ops/s
CacheVsSubstring.shortNameSimple  thrpt    3  12736370.024 ± 1940404.473  ops/s
 */

@BenchmarkMode(Mode.Throughput)
@Warmup(iterations = 3, time = 10)
@Measurement(iterations = 3, time = 10)
@State(Scope.Benchmark)
public class CacheVsSubstring {

    @State(Scope.Thread)
    public static class Config {
        private final Cache<String, String> componentShortNameCache = CacheBuilder.newBuilder()
                .maximumSize(1000)
                .build();
        private final String[] components;

        public Config() {
            components = IntStream.range(0, 4)
                    .mapToObj(i -> RandomStringUtils.randomAlphabetic(10))
                    .toArray(String[]::new);
        }

        String generateComponent() {
            return components[RandomUtils.nextInt(0, components.length)] + "." +
                    components[RandomUtils.nextInt(0, components.length)] + "." +
                    components[RandomUtils.nextInt(0, components.length)] + "." +
                    components[RandomUtils.nextInt(0, components.length)];
        }
    }

    @Benchmark
    public String shortNameCached(Config config) {
        String component = config.generateComponent();
        try {
            return config.componentShortNameCache.get(component, () -> toShortComponent(component));
        } catch (ExecutionException e) {
            throw new RuntimeException(e);
        }
    }

    @Benchmark
    public String shortNameSimple(Config config) {
        return toShortComponent(config.generateComponent());
    }

    private static String toShortComponent(String component) {
        int lastDotIndex = component.lastIndexOf('.');
        if (lastDotIndex <= 0) {
            return component;
        }
        return component.substring(lastDotIndex + 1);
    }

    public static void main(String[] args) throws RunnerException {
        Options opt = new OptionsBuilder()
                .include(CacheVsSubstring.class.getSimpleName())
                .forks(1)
                .build();

        new Runner(opt).run();
    }
}
