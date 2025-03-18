package ru.yandex.ci.jmh;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Function;
import java.util.stream.Collectors;

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

@BenchmarkMode(Mode.Throughput)
@Warmup(iterations = 3, time = 10)
@Measurement(iterations = 3, time = 10)
@State(Scope.Benchmark)
public class CompareToMap {

    List<String> size1 = generate(1);
    List<String> size15 = generate(15);
    List<String> size21 = generate(21);


    @Benchmark
    public Map<String, String> convertWithStream1() {
        return convertStream(size1);
    }

    @Benchmark
    public Map<String, String> convertWithStream15() {
        return convertStream(size15);
    }

    @Benchmark
    public Map<String, String> convertWithStream21() {
        return convertStream(size21);
    }

    @Benchmark
    public Map<String, String> convertWithForAndSize1() {
        return convertForAndSize(size1);
    }

    @Benchmark
    public Map<String, String> convertWithForAndSize15() {
        return convertForAndSize(size15);
    }

    @Benchmark
    public Map<String, String> convertWithForAndSize21() {
        return convertForAndSize(size21);
    }

    private static Map<String, String> convertStream(List<String> list) {
        return list.stream().collect(Collectors.toMap(Function.identity(), Function.identity()));
    }

    private static Map<String, String> convertForAndSize(List<String> list) {
        var map = new HashMap<String, String>(list.size());
        for (var item : list) {
            map.put(item, item);
        }
        return map;
    }

    private static List<String> generate(int size) {
        var list = new ArrayList<String>(size);
        for (int i = 0; i < size; i++) {
            list.add(String.valueOf(i));
        }
        return list;
    }

    public static void main(String[] args) throws RunnerException {
        Options opt = new OptionsBuilder()
                .include(CompareToMap.class.getSimpleName())
//                .addProfiler(GCProfiler.class)
                .forks(1)
                .build();

        new Runner(opt).run();
    }
}
