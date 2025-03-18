package ru.yandex.ci.core.resolver.function;

import java.util.List;

import io.burt.jmespath.Adapter;
import io.burt.jmespath.JmesPathType;
import io.burt.jmespath.function.ArgumentConstraints;
import io.burt.jmespath.function.BaseFunction;
import io.burt.jmespath.function.FunctionArgument;

public class ReplaceFunc extends BaseFunction {
    public ReplaceFunc() {
        super("replace",
                ArgumentConstraints.typeOf(JmesPathType.STRING),
                ArgumentConstraints.typeOf(JmesPathType.STRING),
                ArgumentConstraints.typeOf(JmesPathType.STRING));
    }

    @Override
    protected <T> T callFunction(Adapter<T> runtime, List<FunctionArgument<T>> functionArguments) {
        var string = runtime.toString(functionArguments.get(0).value());
        var from = runtime.toString(functionArguments.get(1).value());
        var to = runtime.toString(functionArguments.get(2).value());

        return runtime.createString(string.replaceAll(from, to));
    }
}
