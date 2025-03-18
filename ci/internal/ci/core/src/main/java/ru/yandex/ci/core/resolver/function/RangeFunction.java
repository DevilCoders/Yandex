package ru.yandex.ci.core.resolver.function;

import java.util.ArrayList;
import java.util.List;

import com.google.common.base.Preconditions;
import io.burt.jmespath.Adapter;
import io.burt.jmespath.JmesPathType;
import io.burt.jmespath.function.ArgumentConstraints;
import io.burt.jmespath.function.BaseFunction;
import io.burt.jmespath.function.FunctionArgument;

public class RangeFunction extends BaseFunction {

    private static final int MAX_NUMBERS = 100;

    public RangeFunction() {
        super(
                "range",
                ArgumentConstraints.typeOf(JmesPathType.NUMBER),
                ArgumentConstraints.typeOf(JmesPathType.NUMBER)
        );
    }

    @Override
    protected <T> T callFunction(Adapter<T> runtime, List<FunctionArgument<T>> arguments) {
        var fromIncluding = runtime.toNumber(arguments.get(0).value()).intValue();
        var toExcluding = runtime.toNumber(arguments.get(1).value()).intValue();

        Preconditions.checkArgument(
                fromIncluding < toExcluding,
                "'from' (first agr) must be smaller than 'to' (second arg). Provided: %s, %s",
                fromIncluding, toExcluding
        );

        int count = toExcluding - fromIncluding;
        Preconditions.checkArgument(
                count <= MAX_NUMBERS,
                "Max numbers allowed numbers is %s. Provided: %s (%s - %s)",
                MAX_NUMBERS, count, toExcluding, fromIncluding
        );
        List<T> numbers = new ArrayList<>(count);
        for (int i = fromIncluding; i < toExcluding; i++) {
            numbers.add(runtime.createNumber(i));
        }
        return runtime.createArray(numbers);
    }
}
