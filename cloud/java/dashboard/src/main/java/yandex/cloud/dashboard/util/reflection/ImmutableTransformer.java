package yandex.cloud.dashboard.util.reflection;

import com.google.common.base.CaseFormat;
import com.google.common.base.Converter;
import com.google.common.base.Preconditions;
import lombok.SneakyThrows;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Objects;
import java.util.stream.Collectors;

import static java.util.stream.Collectors.toMap;
import static yandex.cloud.util.BetterCollectors.throwingMerger;

/**
 * Thread-safe. It's convenient to annotate classes
 * {@link #beanTransformClass()} (Class) transformed as bean}
 * with {@link lombok.With Wither}.
 *
 * @author ssytnik
 */
public abstract class ImmutableTransformer<U> {
    private static final Converter<String, String> CASE_CONVERTER =
            CaseFormat.LOWER_CAMEL.converterTo(CaseFormat.UPPER_CAMEL);

    @SuppressWarnings("unchecked")
    @SneakyThrows
    public <T> T transform(T source) {
        Object result;

        if (source == null) {
            result = null;
        } else if (List.class.isAssignableFrom(source.getClass())) {
            result = ((List<?>) source).stream()
                    .map(this::transform)
                    .collect(Collectors.toList());
        } else if (Map.class.isAssignableFrom(source.getClass())) {
            Map<?, ?> m = (Map<?, ?>) source;
            Preconditions.checkState(m.values().stream().noneMatch(Objects::isNull),
                    "Map should not contain empty values: %s", source);
            result = m.entrySet().stream()
                    .collect(toMap(Entry::getKey, e -> transform(e.getValue()), throwingMerger(), LinkedHashMap::new));
        } else if (applyTransformClass().isAssignableFrom(source.getClass())) {
            result = applyTransform((U) source);
        } else if (beanTransformClass().isAssignableFrom(source.getClass())) {
            Class<?> clazz = source.getClass();
            final List<Field> fields = Reflector.getFieldList(clazz);
            result = source;
            for (Field f : fields) {
                Method method = clazz.getMethod("with" + CASE_CONVERTER.convert(f.getName()), f.getType());
                method.setAccessible(true);
                result = method.invoke(result, transform(f.get(result)));
            }
        } else {
            result = source;
        }

//        // debug
//        if (source != null && !Objects.equals(source, result)) {
//            System.out.printf("Value changed at '%s': '%s -> %s'\n", source.getClass().getSimpleName(), source, result);
//        }

        return Objects.equals(source, result) ? source : (T) result;
    }

    protected abstract Class<U> applyTransformClass();

    protected abstract U applyTransform(U source);

    protected abstract Class beanTransformClass();
}
