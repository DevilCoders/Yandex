package yandex.cloud.dashboard.model.spec.validator;

import lombok.SneakyThrows;
import yandex.cloud.dashboard.util.reflection.Reflector;

import java.lang.reflect.Field;
import java.util.List;
import java.util.Map;

/**
 * @author ssytnik
 */
public interface Validatable<C> {
    @SuppressWarnings("unchecked")
    @SneakyThrows
    static <C> void validateAll(Object source, C context) {
        if (source != null) {
            if (List.class.isAssignableFrom(source.getClass())) {
                ((List<?>) source).forEach(o -> validateAll(o, context));
            } else if (Map.class.isAssignableFrom(source.getClass())) {
                ((Map<?, ?>) source).values().forEach(o -> Validatable.validateAll(o, context));
            } else if (Validatable.class.isAssignableFrom(source.getClass())) {
                Validatable<C> validatable = (Validatable<C>) source;
                C newContext = validatable.newContext(context);
                validatable.validate(newContext);
                for (Field f : Reflector.getFieldList(source.getClass())) {
                    validateAll(f.get(source), newContext);
                }
            }
        }
    }

    default C newContext(C context) {
        return context;
    }

    default void validate(C context) {
    }
}
