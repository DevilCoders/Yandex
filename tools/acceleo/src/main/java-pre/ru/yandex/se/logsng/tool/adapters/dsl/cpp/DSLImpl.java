package ru.yandex.se.logsng.tool.adapters.dsl.cpp;


import ru.yandex.se.logsng.tool.ClassifierCollections;
import ru.yandex.se.logsng.tool.adapters.dsl.*;
import ru.yandex.se.logsng.tool.adapters.dsl.base.ByFieldAggregator;
import ru.yandex.se.logsng.tool.adapters.dsl.base.EventAggregator;
import ru.yandex.se.logsng.tool.adapters.dsl.base.Util;
import ru.yandex.se.logsng.tool.adapters.dsl.cpp.mtl.MtlTypeCpp;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.AbstractStatementVisitor;
import ru.yandex.se.logsng.tool.adapters.dsl.base.MultiLineStatement;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.StatementVisitor;
import ru.yandex.se.logsng.tool.adapters.dsl.statements.*;
import ru.yandex.se.logsng.tool.adapters.dsl.types.*;
import ru.yandex.se.logsng.tool.adapters.dsl.types.Enum;

import java.util.*;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Created by astelmak on 24.05.16.
 */
public class DSLImpl implements DSL {
    private static class CompilationContext {
        static class ObjectContext {
            final LinkedList<ComplexType> objTypes = new LinkedList<>();

            ComplexType getType() {
                return objTypes.isEmpty() ? null : objTypes.getLast();
            }

            void pushType(ComplexType type) {
                objTypes.add(type);
            }

            ComplexType popType() {
                return objTypes.removeLast();
            }
        }

        private final HashMap<ObjStatement, ObjectContext> contextByObj = new HashMap<>();

        ObjectContext getObjectContext(ObjStatement obj) {
            ObjectContext result = contextByObj.get(obj);

            if (result == null)
                contextByObj.put(obj, result = new ObjectContext());

            return result;
        }
    }

    private ru.yandex.se.logsng.tool.adapters.dsl.types.InterfaceAdapter adapterInfo;
    private Collection<Event> events;
    private ThreadLocal<CompilationContext> context = new ThreadLocal<>();

    private static Type getType(TypeKey key) {
        return MtlTypeCpp.getTypeByKey(key);
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
                b.append('(');
                o1.toString(b).append(") && (");

                return o2.toString(b).append(')');
            }

            @Override
            public String toString() {
                return toString(new StringBuilder()).toString();
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
                b.append('(');
                o1.toString(b).append(") || (");

                return o2.toString(b).append(')');
            }

            @Override
            public String toString() {
                return toString(new StringBuilder()).toString();
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

            @Override
            public String toString() {
                return toString(new StringBuilder()).toString();
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
                b.append('(');
                o1.toString(b).append(") == (");

                return o2.toString(b).append(')');
            }

            @Override
            public String toString() {
                return toString(new StringBuilder()).toString();
            }
        };
    }

    @Override
    public Value Add(final Value o1, final Value o2) {
        return new Value() {
            @Override
            public void accept(StatementVisitor visitor) {
                o1.accept(visitor);
                o2.accept(visitor);

                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                b.append('(');
                o1.toString(b).append(") + (");

                return o2.toString(b).append(')');
            }

            @Override
            public String toString() {
                return toString(new StringBuilder()).toString();
            }
        };
    }

    @Override
    public Value Sub(final Value o1, final Value o2) {
        return new Value() {
            @Override
            public void accept(StatementVisitor visitor) {
                o1.accept(visitor);
                o2.accept(visitor);

                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                b.append('(');
                o1.toString(b).append(") + (");

                return o2.toString(b).append(')');
            }

            @Override
            public String toString() {
                return toString(new StringBuilder()).toString();
            }
        };
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
        final Type ev = getType(typeName);
        if (ev == null)
            throw new IllegalArgumentException("Type '" + typeName + "' not found!");

        return new LogicStatement() {
            @Override
            public void accept(StatementVisitor visitor) {
                obj.accept(visitor);

                visitor.visit(this);
            }

            @Override
            public StringBuilder toString(StringBuilder b) {
                if (ev instanceof Event)
                    obj.toString(b).append(".GetScarabType() == ").append(((Event)ev).getTypeCode());
                else
                    obj.toString(b.append("dynamic_cast<").append(ev.getTypeName()).append(">(")).append(") != nullptr");
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

            @Override
            public String toString() {
                return "true";
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

            @Override
            public String toString() {
                return "false";
            }
        };
    }

    @Override
    public Value Nothing() {
        return NothingCppStmt.instance;
    }

    @Override
    public Value Field(final ObjStatement obj, final String name) {
        return new FieldCppStmt(obj, name) {
            @Override
            protected StringBuilder getter(StringBuilder b) {
                ComplexType type = context.get().getObjectContext(getObj()).getType();
                if (type == null) {
                    type = getObj().getType();

                    if (type == null)
                        throw new IllegalArgumentException("Can't get type for object '" + getObj() + "'");
                }

                Field field = type.getFieldByXSDName(name);
                if (field == null)
                    throw new IllegalArgumentException("Field '" + getName() + "' not found in type '" + type.getKey() + "'");

                return b.append(field.getter());
            }
        };
    }

    @Override
    public Value FieldDef(ObjStatement obj, Object ... params) {
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
                return obj.toString(b.append("static_cast<").append(ev.getTypeName()).append("&>(")).append(')');
            }
        };
    }

    private int adapterCounter;
    @Override
    public Value CreateAdapter(final Statement... values) {
        int c = adapterCounter++;

        return new InterfaceAdapterCppStmt(adapterInfo.getTypeName() + (c > 0 ? c : ""), values);
    }

    @Override
    public Value Enum(final String enName) {
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
                return b.append(enumType.getTypeName()).append("::").append(literal);
            }
        };
    }

    private static StringBuilder appendIndent(StringBuilder b, int num) {
        for (int i = 0; i < num; i++)
            b.append("    ");
        return b;
    }

    private StringBuilder defineEvent(int tabs, StringBuilder b, String def) {
        appendIndent(b, tabs).append("#ifdef event\n");
        appendIndent(b, tabs).append("#undef event\n");
        appendIndent(b, tabs).append("#endif\n");

        if (!"event".equals(def))
            appendIndent(b, tabs).append("#define event ").append(def).append('\n');

        return b;
    }


    private void generateCreatorFunc(StringBuilder b, String containerFormat, String adapterName, Map<Event,
                                                    EventAggregator.RequiredType> eventTypes, boolean isPtr) {
        String containerName = String.format(containerFormat, "NCommon::IEvent");
        String s = "I" + adapterInfo.getTypeName() + (isPtr ? "*" : "") + ", " + containerName;

        b.append("#define EFPair ").append("TEventFuncPair<TAdapterProviderFunc<").append(s).append("&>>\n");
        b.append("static const TMaybe<I").append(adapterInfo.getTypeName()).append(isPtr ? "*" : "").append("> Create").append(adapterName).
                append("(").append(containerName).append("& event) {\n");

        b.append("    ").append("static TAdapterProvider<").append(s).append("&, ").
                append(ClassifierCollections.getEvents().size()).append("> provider");

        boolean sep = false;
        for (Map.Entry<Event, EventAggregator.RequiredType> en : eventTypes.entrySet()) {
            if (en.getValue() != EventAggregator.RequiredType.EXCLUDED) {
                if (sep)
                    b.append(",\n");
                else {
                    b.append("{\n");
                    sep = true;
                }

                b.append("        EFPair(").append(en.getKey().getTypeCode()).append(",\n");
                b.append("            [] (").append(containerName).append("& c) {\n");
                b.append("                return ");

                if (isPtr)
                    b.append("static_cast<I").append(adapterInfo.getTypeName()).append("*>(new T").
                        append(adapterName).append("Impl<").
                        append(String.format(containerFormat, en.getKey().getTypeName())).append(">(reinterpret_cast<").
                        append(String.format(containerFormat, en.getKey().getTypeName())).append("&>(c)));");
                else
                    b.append("static_cast<I").append(adapterInfo.getTypeName()).append(">(").append('T').
                        append(adapterName).append("Impl").append(String.format(containerFormat, en.getKey().getTypeName())).
                        append(">(reinterpret_cast<").append(String.format(containerFormat, en.getKey().getTypeName())).append("&>(c)));");

                b.append("\n            })");
            }
        }
        if (sep)
            b.append("\n    }");

        b.append(";\n\n");

        b.append("    return provider.Get(event);");

        b.append("\n}\n#undef EFPair\n\n");
    }

    private void generateSharedPtrSpec(StringBuilder b, String counter, String del) {
        String typeName = adapterInfo.getTypeName();

        b.append("template<>");

        b.append(String.format("\nTHolder<I%s> GetAdapter<I%s>(TSharedPtr<NCommon::IEvent,%s,%s>& event) {\n",
                    typeName, typeName, counter, del));

        b.append(String.format("    auto adapter = Process%s(event);\n", typeName));

        b.append(String.format("    return adapter.Defined() ? THolder<I%s>(*adapter) : THolder<I%s>();\n}\n",
                    typeName, typeName));
    }

    private void generateHolderSpec(StringBuilder b, String del) {
        String typeName = adapterInfo.getTypeName();

        b.append("template<>");

        b.append(String.format("\nTHolder<I%s> GetAdapter<I%s>(THolder<NCommon::IEvent,%s>& event) {\n",
                typeName, typeName, del));

        b.append(String.format("    auto adapter = Process%s(event);\n", typeName));

        b.append(String.format("    return adapter.Defined() ? THolder<I%s>(*adapter) : THolder<I%s>();\n}\n",
                typeName, typeName));
    }

    @Override
    public Statement eval(ObjStatement event) {
        context.set(new CompilationContext());
        adapterCounter = 0;

        try {
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
                final TreeSet<Type> nothingTypesSet = new TreeSet<>();

                header.addStatement(new Statement() {
                    @Override
                    public void accept(StatementVisitor visitor) {
                        adapter.accept(visitor);

                        visitor.visit(this);
                    }

                    @Override
                    public StringBuilder toString(StringBuilder b) {
                        b.append("template<typename TEventContainerType>\nclass T").append(adapter.getName()).
                                append("Impl : public I").append(adapterInfo.getTypeName()).append(" {\n").
                                append("public:\n");

                        b.append("    template<typename TDefContainerType>\n");
                        b.append("    T").append(adapter.getName()).
                                append("Impl(TDefContainerType& evContainer) : EvContainer(evContainer) {}\n\n");

                        b.append("    template<typename TEventType, typename D>\n");
                        b.append("    T").append(adapter.getName()).
                                append("Impl(THolder<TEventType,D>& holder) : EvContainer(holder.Release()) {}\n");

                        b.append("\n    const NCommon::IEvent& GetEvent() " +
                                "const override {\n        return *EvContainer;\n    }\n");

                        if (adapterInfo.getFields().size() != adapter.getParams().length)
                            throw new IllegalArgumentException("Invalid parameters count in createAdapter. Expected=" +
                                    adapterInfo.getFields().size() + ", Actual=" + adapter.getParams().length);

                        final EventAggregator eventAggregator = new EventAggregator();

                        int i = 0;
                        for (final Field field : adapterInfo.getFields()) {
                            ByFieldAggregator fieldAggregator = new ByFieldAggregator();

                            Statement param = adapter.getParams()[i++];

                            NothingCppStmt.pushContext(new NothingCppStmt.Context(
                                        "nothing" + field.getType().getRootTypeName(), field.isOptional()));
                            param.accept(new AbstractStatementVisitor() {
                                @Override
                                public void visit(NothingStatement nothing) {
                                    nothingTypesSet.add(field.getType());
                                }
                            });
                            try {
                                if (param instanceof FieldDefStatement) {
                                    defineEvent(1, b.append('\n'), "event");

                                    b.append("    //---------- ").append(field.getXSDName()).append("\n");

                                    Object[] fParam = ((FieldDefStatement) param).getParams();
                                    if (fParam.length < 1)
                                        throw new IllegalArgumentException("Number of parameters of FieldDef must be >= 2.");
                                    int index = 0;

                                    if (fParam[index] instanceof String) {
                                        fieldAggregator.putEventsWithCompatibleField(field, (String) fParam[index], events);

                                        b.append("\n    template<typename TEventType>");
                                        b.append("\n    inline ").append(field.getType().isSimple()
                                                                            || field.isOptional() ? "" : "const ").
                                                append(field.getType().getTypeName()).
                                                append(" ").append(field.getter()).append("(const TEventType& event) const {\n        ");

                                        ((FieldDefStatement) param).getObj().toString(b.append("return ")).append(".").
                                                append("Get").append(Util.generateName(fParam[index].toString(), Util.CASE.UPPER)).append("();");
                                        b.append("\n    }\n");
                                        index++;
                                    }

                                    for (; index < fParam.length; index++) {
                                        TypeDefStatement def = (TypeDefStatement) fParam[index];

                                        final AtomicBoolean isUsedEventObject = new AtomicBoolean(true);

                                        if (def.getDefinition() instanceof Statement) {
                                            isUsedEventObject.set(false);

                                            ((Statement)def.getDefinition()).accept(new AbstractStatementVisitor() {
                                                @Override
                                                public void visit(ObjStatement obj) {
                                                    if (obj instanceof BaseEventCppStmt)
                                                        isUsedEventObject.set(true);
                                                }
                                            });
                                        }

                                        b.append("\n    inline ").append(field.getType().isSimple()
                                                                            || field.isOptional() ? "" : "const ").
                                                append(field.getType().getTypeName()).
                                                append(" ").append(field.getter()).append("(const ").append(def.getType().getTypeName()).
                                                append('&').append(isUsedEventObject.get() ? " event" : "").append(") const {\n        ");

                                        if (def.getType() instanceof Event)
                                            fieldAggregator.put((Event) def.getType(), EventAggregator.RequiredType.REQUIRED);

                                        if (def.getDefinition() instanceof String) {
                                            b.append("return ");

                                            Field f = def.getType().getFieldByXSDName((String) def.getDefinition());
                                            if (f == null)
                                                throw new IllegalArgumentException("Field '" + def.getDefinition() + "' not found in type '" +
                                                        def.getType() + "'.");
                                            if (!field.getType().equals(f.getType())) {
                                                throw new IllegalArgumentException("Field '" + def.getDefinition() + "' in type '" +
                                                        def.getType() + "' have incompatible type. Expected=" +
                                                        field.getType() + ", Actual=" + f.getType());
                                            }

                                            if (!field.isOptional() && f.isOptional())
                                                throw new IllegalArgumentException("Field '" + def.getDefinition() + "' in type '" +
                                                        def.getType() + "' have incompatible 'optional' attribute.");

                                            ((FieldDefStmtImpl) param).getObj().toString(b).append('.').append(f.getter()).append("();");
                                        } else {
                                            CompilationContext.ObjectContext objectContext = context.get().getObjectContext(((FieldDefStmtImpl) param).getObj());
                                            objectContext.pushType(def.getType());
                                            try {
                                                if (def.getDefinition() instanceof Value)
                                                    ((Value)def.getDefinition()).toString(b.append("return ")).append(';');
                                                else if (def.getDefinition() instanceof Statement)
                                                    ((Statement)def.getDefinition()).toString(b);
                                                else
                                                    throw new IllegalArgumentException("Unknown statement '" + def.getDefinition() + "'");
                                            } finally {
                                                objectContext.popType();
                                            }
                                        }
                                        b.append("\n    }\n");
                                    }

                                    defineEvent(1, b.append("\n"), "(*EvContainer)");
                                    b.append(field.getType().isSimple() || field.isOptional() ? "    " : "    const ").
                                            append(field.getType().getTypeName()).
                                            append(" ").append(field.getter()).append("() const override {\n        ");

                                    b.append("return ").append(field.getter()).append("(event);");
                                    b.append("\n    }\n");

                                } else {
                                    defineEvent(1,b.append("\n"), "(*EvContainer)");
                                    b.append(field.getType().isSimple() || field.isOptional() ? "    " : "    const ").
                                            append(field.getType().getTypeName()).
                                            append(" ").append(field.getter()).append("() const override {\n        ");

                                    if (param instanceof Value) {
                                        param.toString(b.append("return ")).append(';');
                                        if (param instanceof FieldStatement) {
                                            FieldStatement fStmt = (FieldStatement)param;
                                            if (fStmt.getObj().getType() instanceof Event)
                                                fieldAggregator.put((Event)fStmt.getObj().getType(),
                                                                                EventAggregator.RequiredType.REQUIRED);
                                        }
                                    }
                                    else
                                        param.toString(b);
                                    b.append("\n    }");
                                }
                            }
                            catch (Exception ex) {
                                throw new RuntimeException("Error while compile adapter: " + adapterInfo.getTypeName(),
                                            ex);
                            }
                            finally {
                                NothingCppStmt.popContext();
                            }

                            eventAggregator.putAll(fieldAggregator.getEvents());
                        }

                        b.append("\nprivate:\n    TEventContainerType EvContainer;\n");

                        for (Type type : nothingTypesSet)
                            b.append("    static ").append(type.getTypeName()).append(" nothing").
                                    append(type.getRootTypeName()).append(";\n");

                        b.append("};\n#ifdef event\n#undef event\n#endif\n\n");

                        Map<Event, EventAggregator.RequiredType> events = eventAggregator.getEvents();

                        NothingCppStmt.pushContext(new NothingCppStmt.Context("nullptr", true));
                        try {
                            b.append("template<typename D>\n");
                            generateCreatorFunc(b, "THolder<%s,D>", adapter.getName(), events, true);

                            b.append("template<typename C, typename D>\n");
                            generateCreatorFunc(b, "TSharedPtr<%s,C,D>", adapter.getName(), events, true);
                        }
                        finally {
                            NothingCppStmt.popContext();
                        }

                        return b;
                    }

                    @Override
                    public String toString() {
                        return toString(new StringBuilder(16384)).toString();
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
                    context.set(new CompilationContext());
                    try {
                        //noinspection ResultOfMethodCallIgnored
                        header.toString(b);

                        /*
                        Nothing.pushContext(new Nothing.Context("TMaybe<" + adapterInfo.getTypeName() + ">()", true));
                        try {
                            b.append("const TMaybe<I").append(adapterInfo.getTypeName()).append("> GetAdapter<").
                                    append(adapterInfo.getTypeName()).append(">(const IEvent& event) {\n");
                            s.toString(b.append("    ")).append("\n}\n");
                        }
                        finally {
                            Nothing.popContext();
                        }
                        */

                        final AtomicBoolean isUsedEventObject = new AtomicBoolean();

                        s.accept(new AbstractStatementVisitor() {
                            @Override
                            public void visit(ObjStatement obj) {
                                if (obj instanceof BaseEventCppStmt)
                                    isUsedEventObject.set(true);
                            }

                            @Override
                            public void visit(InterfaceAdapterStatement iAdapter) {
                                isUsedEventObject.set(true);
                            }
                        });

                        NothingCppStmt.pushContext(new NothingCppStmt.Context("TMaybe<I" + adapterInfo.getTypeName() + "*>()", true));
                        defineEvent(0, b, "(*evContainer)");
                        try {
                            b.append("template<typename D>");
                            b.append("\nstatic inline TMaybe<I").append(adapterInfo.getTypeName()).
                                append("*> Process").append(adapterInfo.getTypeName()).
                                append("(THolder<NCommon::IEvent,D>&").
                                append(isUsedEventObject.get() ? " evContainer" : "").append(") {\n");
                            b.append("    ");
                            if (s instanceof Value)
                                s.toString(b.append("return ")).append(";\n}\n");
                            else
                                s.toString(b).append("\n}\n");

                            b.append("\ntemplate<typename C, typename D>\n");
                            b.append("static inline TMaybe<I").append(adapterInfo.getTypeName()).
                                append("*> Process").append(adapterInfo.getTypeName()).
                                append("(TSharedPtr<NCommon::IEvent, C, D>&").
                                append(isUsedEventObject.get() ? " evContainer" : "").append(") {\n");
                            b.append("    ");
                            if (s instanceof Value)
                                s.toString(b.append("return ")).append(";\n}\n\n");
                            else
                                s.toString(b).append("\n}\n\n");
                        }
                        finally {
                            defineEvent(0, b, "event").append('\n');
                            NothingCppStmt.popContext();
                        }

                        generateHolderSpec(b, "TDelete");

                        b.append('\n');

                        generateHolderSpec(b, "TNoAction");

                        b.append('\n');

                        generateSharedPtrSpec(b, "TSimpleCounter", "TDelete");

                        b.append('\n');

                        generateSharedPtrSpec(b, "TAtomicCounter", "TDelete");

                        return b;
                    } finally {
                        context.remove();
                    }
                }

                @Override
                public String toString() {
                    return toString(new StringBuilder(8192)).toString();
                }
            };
        }
        finally {
            context.remove();
        }
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
