package ru.yandex.se.logsng.tool.adapters.dsl.statements;


import ru.yandex.se.logsng.tool.adapters.dsl.types.ComplexType;

/**
 * Created by astelmak on 27.05.16.
 */
public class TypeDefStmtImpl implements TypeDefStatement {
    private final ComplexType type;
    private final Object definition;

    public TypeDefStmtImpl(ComplexType type, Object definition) {
        this.type = type;
        this.definition = definition;
    }

    @Override
    public void accept(StatementVisitor visitor) {
        if (definition instanceof Statement)
            ((Statement)definition).accept(visitor);

        visitor.visit(this);
    }

    @Override
    public StringBuilder toString(StringBuilder b) {
        return b;
    }

    @Override
    public ComplexType getType() {
        return type;
    }

    @Override
    public Object getDefinition() {
        return definition;
    }
}
