package ru.yandex.ci.core.resolver.function;

import java.util.List;

import io.burt.jmespath.Adapter;
import io.burt.jmespath.function.ArgumentConstraints;
import io.burt.jmespath.function.BaseFunction;
import io.burt.jmespath.function.FunctionArgument;

// Make sure provided list has at least 1 value, returns same list
// Usage: ${tasks.upstream1.resources[?type = `BUILD_LOGS`] | non_empty(@) | @.id }
public class NotEmptyFunc extends BaseFunction {
    public NotEmptyFunc() {
        super("non_empty", ArgumentConstraints.arrayOf(ArgumentConstraints.anyValue()));
    }

    @Override
    protected <T> T callFunction(Adapter<T> runtime, List<FunctionArgument<T>> arguments) {
        var value = arguments.get(0).value();
        var list = runtime.toList(value);
        if (list.isEmpty()) {
            throw new IllegalStateException("Expect one or more values, found none");
        }
        return value;
    }
}
