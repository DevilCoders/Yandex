package ru.yandex.ci.util;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.concurrent.CompletionException;
import java.util.concurrent.ExecutionException;

import com.google.common.util.concurrent.UncheckedExecutionException;

public class ExceptionUtils {

    private ExceptionUtils() {
    }

    public static boolean isCausedBy(Throwable throwable, Class<? extends Throwable> type) {
        var current = throwable;
        while (current != null) {
            if (type.isAssignableFrom(current.getClass())) {
                return true;
            }
            current = current.getCause();
        }
        return false;
    }

    public static RuntimeException unwrap(Throwable e) throws RuntimeException {
        Throwable t;
        if (
                (e instanceof ExecutionException ||
                        e instanceof UncheckedExecutionException ||
                        e instanceof CompletionException
                ) && e.getCause() != null
        ) {
            t = e.getCause();
        } else {
            t = e;
        }
        if (t instanceof RuntimeException) {
            return (RuntimeException) t;
        } else {
            return new RuntimeException(t);
        }
    }

    // copy-paste from commons-lang
    public static String getStackTrace(Throwable throwable) {
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw, true);
        throwable.printStackTrace(pw);
        return sw.getBuffer().toString();
    }
}
