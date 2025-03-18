package ru.yandex.ci.core.resolver.function;

import java.util.List;
import java.util.function.Supplier;

import com.google.gson.JsonObject;
import io.burt.jmespath.Adapter;
import io.burt.jmespath.function.BaseFunction;
import io.burt.jmespath.function.FunctionArgument;

public class RootFunc extends BaseFunction {
    private final Supplier<JsonObject> rootSupplier;

    public RootFunc(Supplier<JsonObject> rootSupplier) {
        super("root");
        this.rootSupplier = rootSupplier;
    }

    @SuppressWarnings("unchecked")
    @Override
    protected <T> T callFunction(Adapter<T> runtime, List<FunctionArgument<T>> functionArguments) {
        return (T) rootSupplier.get();
    }
}
