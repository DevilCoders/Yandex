package ru.yandex.ci.core.resolver.function;

import java.util.List;

import io.burt.jmespath.Adapter;
import io.burt.jmespath.JmesPathType;
import io.burt.jmespath.function.ArgumentConstraints;
import io.burt.jmespath.function.BaseFunction;
import io.burt.jmespath.function.FunctionArgument;

public class ToJsonFunc extends BaseFunction {
    public ToJsonFunc() {
        super("to_json",
                ArgumentConstraints.typeOf(JmesPathType.STRING));
    }

    @Override
    protected <T> T callFunction(Adapter<T> runtime, List<FunctionArgument<T>> functionArguments) {
        var json = runtime.toString(functionArguments.get(0).value());
        return runtime.parseString(json);
    }
}
