package yandex.cloud.util;

import java.util.Objects;
import java.util.function.Predicate;

/**
 * @author entropia
 */
public final class Predicates {
    private Predicates() {
    }

    /**
     * Tests whether only one object (either {@code a} or {@code b}) matches the specified predicate {@code p}
     * (<em>i.e.</em>, whether {@code p(a) != p(b)}).
     * <p>For example,
     * <blockquote><pre>
     * checkState(oneOf(r.getA(), r.getB(), Objects::notNull), "either A or B should be present");
     * </pre></blockquote>
     *
     * @param a   first object
     * @param b   second object
     * @param p   predicate
     * @param <T> object type
     * @return {@code true} if and only if only one of the objects (either {@code a} or {@code b})
     * matches the specified {@code predicate}; {@code false} otherwise
     */
    public static <T> boolean oneOf(T a, T b, Predicate<? super T> p) {
        return p.test(a) != p.test(b);
    }

    public static <T> boolean oneOf(T a, T b) {
        return oneOf(a, b, Objects::nonNull);
    }
}
