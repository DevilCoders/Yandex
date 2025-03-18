package ru.yandex.ci.core.resolver.function;

import java.util.List;

import io.burt.jmespath.Adapter;
import io.burt.jmespath.JmesPathType;
import io.burt.jmespath.function.ArgumentConstraints;
import io.burt.jmespath.function.BaseFunction;
import io.burt.jmespath.function.FunctionArgument;

// Join any list into string (separated with any value), returns string
// Usage: ${tasks.upstream1.resources[?type = `BUILD_LOGS`].id | join_any(', ', @)}
public class JoinAnyFunc extends BaseFunction {

    public JoinAnyFunc() {
        super("join_any",
                ArgumentConstraints.typeOf(JmesPathType.STRING),
                ArgumentConstraints.arrayOf(ArgumentConstraints.anyValue()));
    }

    @Override
    protected <T> T callFunction(Adapter<T> runtime, List<FunctionArgument<T>> arguments) {
        var separator = arguments.get(0).value();
        var value = arguments.get(1).value();

        var listIter = runtime.toList(value).iterator();
        if (listIter.hasNext()) {
            String separatorStr = runtime.toString(separator);

            var buffer = new StringBuilder();
            buffer.append(runtime.toString(listIter.next()));
            while (listIter.hasNext()) {
                buffer.append(separatorStr);
                buffer.append(runtime.toString(listIter.next()));
            }
            return runtime.createString(buffer.toString());
        } else {
            return runtime.createString("");
        }
    }
}
