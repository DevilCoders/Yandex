package ru.yandex.ci.core.resolver.function;

import java.util.List;
import java.util.function.Function;

import io.burt.jmespath.Adapter;
import io.burt.jmespath.JmesPathType;
import io.burt.jmespath.function.ArgumentConstraints;
import io.burt.jmespath.function.BaseFunction;
import io.burt.jmespath.function.FunctionArgument;

public class CaseFunction extends BaseFunction {
    private final Function<String, String> action;

    public CaseFunction(String name, Function<String, String> action) {
        super(
                name,
                ArgumentConstraints.typeOf(JmesPathType.STRING)
        );
        this.action = action;
    }

    @Override
    protected <T> T callFunction(Adapter<T> runtime, List<FunctionArgument<T>> arguments) {
        var string = runtime.toString(arguments.get(0).value());
        return runtime.createString(action.apply(string));
    }
}
