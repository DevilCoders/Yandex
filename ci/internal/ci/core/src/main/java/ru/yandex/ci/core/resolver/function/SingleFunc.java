package ru.yandex.ci.core.resolver.function;

import java.util.List;

import io.burt.jmespath.Adapter;
import io.burt.jmespath.function.ArgumentConstraints;
import io.burt.jmespath.function.BaseFunction;
import io.burt.jmespath.function.FunctionArgument;

// Make sure provided list has exactly 1 value, returns element (not a list)
// Usage: ${tasks.upstream1.resources[?type = `BUILD_LOGS`] | single(@) | @.id }
public class SingleFunc extends BaseFunction {
    public SingleFunc() {
        super("single", ArgumentConstraints.arrayOf(ArgumentConstraints.anyValue()));
    }

    @Override
    protected <T> T callFunction(Adapter<T> runtime, List<FunctionArgument<T>> arguments) {
        var list = runtime.toList(arguments.get(0).value());
        if (list.isEmpty()) {
            throw new IllegalStateException("Expect single value, found none");
        } else if (list.size() == 1) {
            return list.get(0);
        } else {
            throw new IllegalStateException("Expect single value, found " + list.size());
        }
    }
}
