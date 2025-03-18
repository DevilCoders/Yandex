package ru.yandex.se.logsng.tool.adapters.dsl.java;

import ru.yandex.se.logsng.tool.adapters.dsl.DSL;
import ru.yandex.se.logsng.tool.adapters.dsl.base.*;
import ru.yandex.se.logsng.tool.adapters.dsl.mtl.MtlType;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.*;
import ru.yandex.se.logsng.tool.adapters.dsl.types.*;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Enum;

import java.util.Collection;
import java.util.LinkedList;
import java.util.Map;

/**
 * Created by astelmak on 12.07.16.
 */
public class DSLImpl implements DSL {

    private ru.yandex.se.logsng.tool.adapters.dsl.types.InterfaceAdapter adapterInfo;
    private Collection<Event> events;

    private static Type getType(TypeKey key) {
        return MtlType.getTypeByKey(key);
    }

    private Type getType(String typeName) {
        int i = typeName.indexOf('.');

        if (i >= 0)
            return getType(new TypeKey(typeName.substring(i + 1), typeName.substring(0, i)));

        Type ev = getType(new TypeKey(typeName, adapterInfo.getKey().getModule()));

        return ev == null ? getType(new TypeKey(typeName, "common")) : ev;
    }

    private Event getEvent(String typeName) {
        Type result = getType(typeName);

        return result != null && (result instanceof Event) ? (Event)result : null;
    }

    @Override
    public LogicStatement And(final LogicStatement o1, final LogicStatement o2) {
        return new LogicStatement() {
            @Override
            public void accept(StatementVisitor visitor) {
                o1.accept(visitor);

                o2.accept(visitor);

                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                return o2.toString(o1.toString(b.append('(').append(") && ("))).append(')');
            }
        };
    }

    @Override
    public LogicStatement Or(final LogicStatement o1, final LogicStatement o2) {
        return new LogicStatement() {
            @Override
            public void accept(StatementVisitor visitor) {
                o1.accept(visitor);

                o2.accept(visitor);

                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                return o2.toString(o1.toString(b.append('(').append(") || ("))).append(')');
            }
        };
    }

    @Override
    public LogicStatement Not(final LogicStatement o) {
        return new LogicStatement() {
            @Override
            public void accept(StatementVisitor visitor) {
                o.accept(visitor);

                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                return o.toString(b.append("!(")).append(')');
            }
        };
    }

    @Override
    public LogicStatement Eq(final Value o1, final Value o2) {
        return new LogicStatement() {
            @Override
            public void accept(StatementVisitor visitor) {
                o1.accept(visitor);

                o2.accept(visitor);

                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                return o2.toString(o1.toString(b.append('(')).append(".equals(")).append(')');
            }
        };
    }

    @Override
    public Value Add(Value o1, Value o2) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Value Sub(Value o1, Value o2) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Value Value(final Object obj) {
        return new Value() {
            @Override
            public void accept(StatementVisitor visitor) {
                if (obj instanceof Statement)
                    ((Statement)obj).accept(visitor);

                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                return obj instanceof Statement ? ((Statement)obj).toString(b) : b.append(obj);
            }
        };
    }

    @Override
    public LogicStatement InstanceOf(final ObjStatement obj, String typeName) {
        final Type type = getType(typeName);
        if (type == null)
            throw new IllegalArgumentException("Type '" + typeName + "' not found!");

        return new LogicStatement() {
            @Override
            public void accept(StatementVisitor visitor) {
                obj.accept(visitor);

                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                if (type instanceof Event)
                    obj.toString(b).append(".type() == ").append(((Event)type).getTypeCode());
                else
                    obj.toString(b.append('(')).append(") instanceof ").append(type.getTypeName());
                return b;
            }

            @Override
            public String toString() {
                return toString(new StringBuilder()).toString();
            }
        };

    }

    @Override
    public LogicStatement True() {
        return new LogicStatement() {
            @Override
            public void accept(StatementVisitor visitor) {
                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                return b.append("true");
            }
        };
    }

    @Override
    public LogicStatement False() {
        return new LogicStatement() {
            @Override
            public void accept(StatementVisitor visitor) {
                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                return b.append("false");
            }
        };
    }

    @Override
    public Value Nothing() {
        return new Value() {
            @Override
            public void accept(StatementVisitor visitor) {
                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                return b.append("null");
            }
        };
    }

    @Override
    public Value Field(final ObjStatement val, final String name) {
        final Field field = val.getType().getFieldByXSDName(name);
        if (field == null)
            throw new IllegalArgumentException("Field '" + name + "' not found in type '" + val.getType().getKey() + "'");

        return new FieldStatement() {
            @Override
            public ObjStatement getObj() {
                return val;
            }

            @Override
            public String getName() {
                return name;
            }

            @Override
            public void accept(StatementVisitor visitor) {
                val.accept(visitor);

                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                return val.toString(b).append('.').append(field.getter()).append("()");
            }
        };
    }

    @Override
    public Value FieldDef(ObjStatement obj, Object... params) {
        return new FieldDefStmtImpl(obj, params);
    }

    @Override
    public Statement TypeDef(String typeName, Object definition) {
        Event ev = getEvent(typeName);

        if (ev == null)
            throw new IllegalArgumentException("Event with type '" + typeName + "' not found!");

        return new TypeDefStmtImpl(ev, definition);
    }

    @Override
    public Statement If(final LogicStatement condition, final Statement body, final Statement Else) {
        return new Statement() {
            @Override
            public void accept(StatementVisitor visitor) {
                condition.accept(visitor);
                body.accept(visitor);
                Else.accept(visitor);

                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                int tabs = Util.getTabsOnTail(b);

                b.append("if (");

                condition.toString(b).append(") {\n");

                body.toString(Util.appendTabs(b, tabs + 1)).append('\n');
                Util.appendTabs(b, tabs).append("}\n");

                Util.appendTabs(b, tabs).append("else {\n");
                Else.toString(Util.appendTabs(b, tabs + 1)).append('\n');
                Util.appendTabs(b, tabs).append("}");

                return b;
            }
        };
    }

    @Override
    public Statement Return(final Value val) {
        return new Statement() {
            @Override
            public void accept(StatementVisitor visitor) {
                val.accept(visitor);

                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                return val.toString(b.append("return ")).append(';');
            }
        };
    }

    @Override
    public ObjStatement Cast(final ObjStatement obj, String typeName) {
        final Type ev = getType(typeName);

        if (ev == null || !(ev instanceof ComplexType))
            throw new IllegalArgumentException("Type '" + typeName + "' not found!");

        return new ObjStatement() {
            @Override
            public ComplexType getType() {
                return (ComplexType) ev;
            }

            @Override
            public void accept(StatementVisitor visitor) {
                obj.accept(visitor);

                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                return obj.toString(b.append("((").append(ev.getTypeName()).append(')')).append(')');
            }
        };

    }

    private int adapterCounter;
    @Override
    public Value CreateAdapter(Statement... values) {
        int c = adapterCounter++;

        return new InterfaceAdapterJavaStmt(adapterInfo.getRootTypeName() + (c > 0 ? c : ""), values);
    }

    @Override
    public Value Enum(String enName) {
        int enIndex = enName.lastIndexOf('.');

        if (enIndex == -1)
            throw new IllegalArgumentException("Invalid enum format '" + enName + "'");

        String typeName = enName.substring(0, enIndex);

        Type type = getType(typeName);

        if (type == null)
            throw new IllegalArgumentException("Type '" + typeName + "' not found.");

        if (!(type instanceof Enum))
            throw new IllegalArgumentException("Type '" + typeName + "' is not enum.");

        final Enum enumType = (Enum)type;

        final String literal = enName.substring(enIndex + 1);

        if (!enumType.getEnumLiterals().contains(literal))
            throw new IllegalArgumentException("Type '" + typeName + "' is not contain '" + literal + "'.");

        return new Value() {
            @Override
            public void accept(StatementVisitor visitor) {
                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                return b.append(enumType.getTypeName()).append('.').append(literal);
            }
        };

    }

    @Override
    public Statement eval(ObjStatement event) {
        adapterCounter = 0;

        final Statement s = evalImpl(event);

        final LinkedList<InterfaceAdapterStatement> adapters = new LinkedList<>();
        s.accept(new AbstractStatementVisitor() {
            @Override
            public void visit(InterfaceAdapterStatement iAdapter) {
                adapters.add(iAdapter);
            }
        });

        final MultiLineStatement header = new MultiLineStatement();

        for (final InterfaceAdapterStatement adapter : adapters) {
            header.addStatement(new Statement() {
                @Override
                public void accept(StatementVisitor visitor) {
                    adapter.accept(visitor);

                    visitor.visit(this);
                }

                @Override
                public StringBuilder toString(StringBuilder b) {
                    if (adapterInfo.getFields().size() != adapter.getParams().length)
                        throw new IllegalArgumentException("Invalid parameters count in createAdapter. Expected=" +
                                adapterInfo.getFields().size() + ", Actual=" + adapter.getParams().length);

                    final EventAggregator aggregator = new EventAggregator();

                    int i = 0;
                    for (final Field field : adapterInfo.getFields()) {
                        ByFieldAggregator fieldAggregator = new ByFieldAggregator();

                        Statement param = adapter.getParams()[i++];

                        if (param instanceof FieldDefStatement) {
                            Object[] fParam = ((FieldDefStatement) param).getParams();
                            if (fParam.length < 1)
                                throw new IllegalArgumentException("Number of parameters of FieldDef must be >= 2.");

                            int index = 0;
                            if (fParam[index] instanceof String)
                                fieldAggregator.putEventsWithCompatibleField(field, (String) fParam[index++], events);

                            for (; index < fParam.length; index++) {
                                TypeDefStatement def = (TypeDefStatement) fParam[index];

                                if (def.getType() instanceof Event)
                                    fieldAggregator.put((Event) def.getType(), EventAggregator.RequiredType.REQUIRED);
                            }
                        }
                        else if (param instanceof FieldStatement) {
                            FieldStatement fStmt = (FieldStatement)param;
                            if (fStmt.getObj().getType() instanceof Event)
                                fieldAggregator.put((Event)fStmt.getObj().getType(),
                                        EventAggregator.RequiredType.REQUIRED);
                        }

                        aggregator.putAll(fieldAggregator.getEvents());
                    }

                    int baseTabs = 1;

                    String arrayName = "providers" + adapter.getName();
                    Util.appendTabs(b, baseTabs).append("private static final AdapterProvider[] ").
                            append(arrayName).append(" = new AdapterProvider[EventType.values().length];\n");
                    Util.appendTabs(b, baseTabs).append("static {\n");
                    for (Map.Entry<Event, EventAggregatorStrategy.RequiredType> en : aggregator.getEvents().entrySet()) {
                        if (en.getValue() != EventAggregatorStrategy.RequiredType.EXCLUDED) {
                            Util.appendTabs(b, baseTabs + 1).append(arrayName).append('[').append(en.getKey().getTypeCode()).
                                    append(".ordinal()] = ").append("new AdapterProvider() {\n");
                            int tabs = baseTabs + 2;

                            Util.appendTabs(b, tabs).append("@Override\n");
                            Util.appendTabs(b, tabs).append("public ").append(adapterInfo.getTypeName()).append(" provide(Event ev) {\n");

                            tabs++;
                            Util.appendTabs(b, tabs).append("return new AdapterBase<").append(en.getKey().getTypeName()).
                                    append(">((").append(en.getKey().getTypeName()).append(") ev) {\n");

                            tabs++;
                            i = 0;
                            for (final Field field : adapterInfo.getFields()) {
                                Statement param = adapter.getParams()[i++];

                                Util.appendTabs(b, tabs).append("@Override\n");
                                Util.appendTabs(b, tabs).append("public ").append(field.getType().getTypeName()).append(' ').
                                        append(field.getter()).append("() {\n");
                                if (param instanceof FieldDefStatement) {
                                    Object[] fParam = ((FieldDefStatement) param).getParams();

                                    boolean already_used = false;

                                    for (int index = fParam[0] instanceof String ? 1 : 0; index < fParam.length; index++) {
                                        TypeDefStatement def = (TypeDefStatement) fParam[index];

                                        if (def.getType().equals(en.getKey())) {
                                            if (def.getDefinition() instanceof String) {
                                                Util.appendTabs(b, tabs + 1).append("return event.").
                                                        append(en.getKey().getFieldByXSDName((String)def.getDefinition()).getter())
                                                        .append("();\n");
                                            }
                                            else {
                                                if (def.getDefinition() instanceof Value)
                                                    ((Value)def.getDefinition()).toString(Util.appendTabs(b, tabs + 1).append("return ")).append(";\n");
                                                else if (def.getDefinition() instanceof Statement)
                                                    ((Statement)def.getDefinition()).toString(Util.appendTabs(b, tabs + 1));
                                                else
                                                    throw new IllegalArgumentException("Unknown statement '" + def.getDefinition() + "'");
                                            }
                                            already_used = true;
                                            break;
                                        }
                                    }

                                    if (!already_used && fParam[0] instanceof String) {
                                        Util.appendTabs(b, tabs + 1).append("return event.").
                                            append(en.getKey().getFieldByXSDName((String)fParam[0]).getter()).append("();\n");
                                    }
                                }
                                else {
                                    if (param instanceof Value) {
                                        param.toString(Util.appendTabs(b, tabs + 1).append("return ")).append(";\n");
                                    }
                                    else
                                        param.toString(Util.appendTabs(b, tabs + 1));
                                }

                                Util.appendTabs(b, tabs).append("}\n");
                            }

                            Util.appendTabs(b, --tabs).append("};\n");
                            Util.appendTabs(b, --tabs).append("}\n");
                            Util.appendTabs(b, --tabs).append("};\n");
                        }
                    }

                    Util.appendTabs(b, baseTabs).append("}\n");

                    Util.appendTabs(b, baseTabs).append("private static ").append(adapterInfo.getTypeName()).
                            append(" provide").append(adapter.getName()).append("(Event event) {\n");
                    Util.appendTabs(b, baseTabs + 1).append("final AdapterProvider provider = ").append(arrayName).
                            append("[event.type().ordinal()];\n");
                    Util.appendTabs(b, baseTabs + 1).append("return provider == null ? null : provider.provide(event);\n");
                    Util.appendTabs(b, baseTabs).append("}\n\n");

                    return b;
                }
            });
        }

        return new Statement() {
            @Override
            public void accept(StatementVisitor visitor) {
                header.accept(visitor);

                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                //noinspection ResultOfMethodCallIgnored
                header.toString(b);

                int tabs = 1;

                Util.appendTabs(b, tabs).append("public static ").append(adapterInfo.getTypeName()).
                        append(" extract(Event event) {\n");
                Util.appendTabs(b, tabs + 1);

                if (s instanceof Value)
                    s.toString(b.append("return ")).append(";\n");
                else
                    s.toString(b).append('\n');

                Util.appendTabs(b, tabs).append("}\n");

                return b;
            }
        };
    }

    @Override
    public Statement evalImpl(ObjStatement event) {
        throw new UnsupportedOperationException("evalImpl() not implemented");
    }

    @Override
    public void setParameters(Object... params) {
        if (params.length != 2)
            throw new IllegalArgumentException("Length of parameters must be == 2");

        adapterInfo = (InterfaceAdapter)params[0];

        events = (Collection<Event>) params[1];
    }
}
