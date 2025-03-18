package ru.yandex.ci.common.temporal;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;

public class TemporalParamsSerializerInvocationHandler implements InvocationHandler {
    @Nullable
    private Method method;
    @Nullable
    private Object[] args;

    public TemporalParamsSerializerInvocationHandler() {
    }

    @Nullable
    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        if (method.equals(Object.class.getMethod("toString"))) {
            // TODO: workflow info
            return "TemporalParamsSerializerInvocationHandler";
        }
        this.method = method;
        this.args = args;
        return null;
    }

    public Method getMethod() {
        Preconditions.checkState(method != null, "Not called");
        return method;
    }

    public Object[] getArgs() {
        Preconditions.checkState(args != null, "Not called");
        return args;
    }
}
