package ru.yandex.ci.core.resolver.function;

import java.util.List;

import io.burt.jmespath.Adapter;
import io.burt.jmespath.JmesPathType;
import io.burt.jmespath.function.ArgumentConstraints;
import io.burt.jmespath.function.BaseFunction;
import io.burt.jmespath.function.FunctionArgument;

public class AddFunc extends BaseFunction {
    public AddFunc() {
        super("add",
                ArgumentConstraints.typeOf(JmesPathType.NUMBER),
                ArgumentConstraints.typeOf(JmesPathType.NUMBER));
    }

    @Override
    protected <T> T callFunction(Adapter<T> runtime, List<FunctionArgument<T>> arguments) {
        var first = runtime.toNumber(arguments.get(0).value());
        var second = runtime.toNumber(arguments.get(1).value());
        return runtime.createNumber(first.longValue() + second.longValue());
    }
}
