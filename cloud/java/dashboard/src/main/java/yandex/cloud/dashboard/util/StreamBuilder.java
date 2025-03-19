package yandex.cloud.dashboard.util;

import lombok.Value;

import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * @author ssytnik
 */
@Value(staticConstructor = "create")
public class StreamBuilder<T> {
    Stream.Builder<T> stream = Stream.builder();

    @SafeVarargs
    public final StreamBuilder<T> add(T... ts) {
        Arrays.stream(ts).forEach(stream::add);
        return this;
    }

    @SafeVarargs
    public final StreamBuilder<T> addIf(boolean condition, T... ts) {
        return condition ? add(ts) : this;
    }

    public Stream<T> build() {
        return stream.build();
    }

    public List<T> toList() {
        return build().collect(Collectors.toList());
    }

    public String toString(String delimiter) {
        return build().map(Object::toString).collect(Collectors.joining(delimiter));
    }
}
