package ru.yandex.se.logsng.tool.adapters.dsl.cpp;


import ru.yandex.se.logsng.tool.adapters.dsl.statements.StatementVisitor;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.NothingStatement;

import java.util.LinkedList;

/**
 * Created by astelmak on 01.06.16.
 */
public class NothingCppStmt implements NothingStatement {
    public static class Context {
        final String data;
        final boolean enabled;

        public Context(String data, boolean enabled) {
            this.data = data;
            this.enabled = enabled;
        }
    }

    private static final ThreadLocal<LinkedList<Context>> context = new ThreadLocal<LinkedList<Context>>() {
        @Override
        protected LinkedList<Context> initialValue() {
            return new LinkedList<>();
        }
    };

    public static void pushContext(Context ctx) {
        context.get().add(ctx);
    }

    public static Context popContext() {
        if (context.get().isEmpty())
            throw new IllegalStateException("Context stack is empty.");
        return context.get().removeLast();
    }

    public static final NothingCppStmt instance = new NothingCppStmt();

    private NothingCppStmt() {
    }

    @Override
    public void accept(StatementVisitor visitor) {
        visitor.visit(this);
    }

    @Override
    public StringBuilder toString(StringBuilder b) {
        if (context.get().isEmpty())
            return b.append("nullptr");

        Context ctx = context.get().getLast();

        if (!ctx.enabled)
            throw new IllegalStateException("'Nothing' must defined in 'optional' context");

        return b.append(context.get().getLast().data);
    }

    @Override
    public String toString() {
        return toString(new StringBuilder()).toString();
    }
}
