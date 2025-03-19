package yandex.cloud.dashboard.model.spec;

import com.google.common.base.MoreObjects;
import com.google.common.base.Preconditions;
import yandex.cloud.dashboard.util.reflection.ImmutableTransformer;
import yandex.cloud.util.Strings;

import java.util.Map;
import java.util.regex.Pattern;

import static yandex.cloud.util.Predicates.oneOf;

/**
 * Since {@link Resolvable} works via <code>with...</code> methods now,
 * it's convenient to annotate implementation class with <code>Wither</code>
 *
 * @author ssytnik
 */
public interface Resolvable {
    Pattern EXPRESSION = Pattern.compile("@([\\w:]+)|@\\{([\\w:]+)}");

    static <T> T resolve(T source, Map<String, String> replacementMap) {
        return new ImmutableTransformer<String>() {
            @Override
            protected Class<String> applyTransformClass() {
                return String.class;
            }

            @Override
            protected String applyTransform(String source) {
                return Strings.replace(source, EXPRESSION, m -> {
                    Preconditions.checkState(oneOf(m.group(1), m.group(2)));
                    String name = MoreObjects.firstNonNull(m.group(1), m.group(2));
                    return replacementMap.getOrDefault(name, m.group());
                });
            }

            @Override
            protected Class beanTransformClass() {
                return Resolvable.class;
            }
        }.transform(source);
    }

}
