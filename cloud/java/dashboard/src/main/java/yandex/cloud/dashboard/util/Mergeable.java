package yandex.cloud.dashboard.util;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableList.Builder;

import java.util.Iterator;
import java.util.List;
import java.util.function.BinaryOperator;

/**
 * TODO optimization: merger should reuse higher precedence object if it doesn't change
 *
 * @author ssytnik
 */
public interface Mergeable<T extends Mergeable<T>> {

    static <T extends Mergeable<T>> T mergeNullable(T higherPrecedence, T lowerPrecedence) {
        return mergeNullable(higherPrecedence, lowerPrecedence, Mergeable::merge);
    }

    static <T extends Mergeable<T>> List<T> mergeNullable(List<T> higherPrecedence, List<T> lowerPrecedence) {
        return mergeNullable(higherPrecedence, lowerPrecedence, Mergeable::mergeLists);
    }

    static <T> T mergeNullable(T higherPrecedence, T lowerPrecedence, BinaryOperator<T> merger) {
        if (higherPrecedence == null) {
            return lowerPrecedence;
        } else if (lowerPrecedence == null) {
            return higherPrecedence;
        } else {
            return merger.apply(higherPrecedence, lowerPrecedence);
        }
    }

    T merge(T lowerPrecedence);

    static <T extends Mergeable<T>> List<T> mergeLists(List<T> higherPrecedence, List<T> lowerPrecedence) {
        Builder<T> b = ImmutableList.builder();
        Iterator<T> hi = higherPrecedence.iterator();
        Iterator<T> li = lowerPrecedence.iterator();
        while (hi.hasNext() || li.hasNext()) {
            b.add(mergeNullable((hi.hasNext() ? hi.next() : null), (li.hasNext() ? li.next() : null)));
        }
        return b.build();
    }
}
