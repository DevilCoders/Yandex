package ru.yandex.ci.util;

import java.util.Collection;
import java.util.List;
import java.util.function.Consumer;
import java.util.function.Function;

import javax.annotation.Nullable;
import javax.annotation.ParametersAreNonnullByDefault;

import com.google.common.base.Preconditions;

/**
 * Реализация стратегии переопределения свойств в бинах.
 */
@ParametersAreNonnullByDefault
public class Overrider<Target> {
    private final Target base;
    private final Target override;

    public Overrider(Target base, Target override) {
        this.base = base;
        this.override = override;
    }

    public static <T extends Overridable<T>> T overrideNullable(@Nullable T base, @Nullable T override) {
        if (base != null && override != null) {
            return base.override(override);
        } else if (base != null) {
            return base;
        }
        return override;
    }

    public <T, V extends Collection<?>> void collection(Consumer<V> setter, Function<Target, V> getter) {
        Preconditions.checkNotNull(setter);
        Preconditions.checkNotNull(getter);

        V fromOverride = getter.apply(override);

        if (fromOverride != null) {
            setter.accept(fromOverride);
            return;
        }

        V fromBase = getter.apply(base);
        if (fromBase != null) {
            setter.accept(fromBase);
        }
    }

    public <T> void extendDistinctList(Consumer<List<T>> setter, Function<Target, List<T>> getter) {
        Preconditions.checkNotNull(setter);
        Preconditions.checkNotNull(getter);
        var fromBase = getter.apply(base);
        var fromOverride = getter.apply(override);
        setter.accept(CollectionUtils.extendDistinct(fromBase, fromOverride));
    }

    public <T> void collectionByOne(Consumer<T> setter, Function<Target, ? extends Collection<T>> getter) {
        Preconditions.checkNotNull(setter);
        Preconditions.checkNotNull(getter);
        Collection<T> fromOverride = getter.apply(override);
        if (fromOverride != null) {
            fromOverride.forEach(setter);
            return;
        }

        Collection<T> fromBase = getter.apply(base);
        if (fromBase != null) {
            fromBase.forEach(setter);
        }
    }

    public <T extends Overridable<T>> void fieldDeep(Consumer<T> setter, Function<Target, T> getter) {
        Preconditions.checkNotNull(setter);
        Preconditions.checkNotNull(getter);

        T fromOverride = getter.apply(override);
        T fromBase = getter.apply(base);

        if (fromOverride != null && fromBase != null) {
            setter.accept(fromBase.override(fromOverride));
        } else if (fromOverride != null) {
            setter.accept(fromOverride);
        } else if (fromBase != null) {
            setter.accept(fromBase);
        }
    }

    public <T> void field(Consumer<T> setter, Function<Target, T> getter) {
        Preconditions.checkNotNull(setter);
        Preconditions.checkNotNull(getter);

        T fromOverride = getter.apply(override);
        if (fromOverride != null) {
            setter.accept(fromOverride);
            return;
        }

        T fromBase = getter.apply(base);
        if (fromBase != null) {
            setter.accept(fromBase);
        }
    }
}
