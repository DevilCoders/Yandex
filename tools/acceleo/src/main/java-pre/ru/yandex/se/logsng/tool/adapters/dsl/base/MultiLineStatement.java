package ru.yandex.se.logsng.tool.adapters.dsl.base;


import ru.yandex.se.logsng.tool.adapters.dsl.statements.Statement;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.StatementVisitor;

import java.util.LinkedList;

/**
 * Created by astelmak on 25.05.16.
 */
public class MultiLineStatement implements Statement {
    private final LinkedList<Statement> subStatemets = new LinkedList<>();
    private final String separator, header, footer;

    public MultiLineStatement() {
        this("", "", "");
    }

    public MultiLineStatement(String separator, String header, String footer) {
        this.separator = separator;
        this.header = header;
        this.footer = footer;
    }

    public void addStatement(Statement s) {
        subStatemets.add(s);
    }

    @Override
    public void accept(StatementVisitor visitor) {
        for (Statement s : subStatemets)
            s.accept(visitor);

        visitor.visit(this);
    }

    @Override
    public StringBuilder toString(StringBuilder b) {
        b.append(header);

        for (Statement s : subStatemets)
            s.toString(b).append(separator);

        return b.append(footer);
    }

    @Override
    public String toString() {
        return toString(new StringBuilder()).toString();
    }
}
