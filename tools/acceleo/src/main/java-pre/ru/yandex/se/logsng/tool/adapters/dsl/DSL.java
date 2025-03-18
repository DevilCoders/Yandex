package ru.yandex.se.logsng.tool.adapters.dsl;

import ru.yandex.se.logsng.tool.adapters.dsl.statements.*;

/**
 * Created by astelmak on 24.05.16.
 */
public interface DSL {
    LogicStatement And(LogicStatement o1, LogicStatement o2);

    LogicStatement Or(LogicStatement o1, LogicStatement o2);

    LogicStatement Not(LogicStatement o);

    LogicStatement Eq(Value o1, Value o2);

    Value Add(Value o1, Value o2);

    Value Sub(Value o1, Value o2);

    Value Value(Object obj);

    LogicStatement InstanceOf(ObjStatement obj, String typeName);

    LogicStatement True();

    LogicStatement False();

    Value Nothing();

    Value Field(ObjStatement val, String name);

    Value FieldDef(ObjStatement val, Object ... params);

    Statement TypeDef(String type, Object definition);

    Statement If(LogicStatement condition, Statement body, Statement Else);

    Statement Return(Value val);

    ObjStatement Cast(ObjStatement obj, String typeName);

    Value CreateAdapter(Statement ... values);

    Value Enum(String en);


    Statement eval(ObjStatement event);

    Statement evalImpl(ObjStatement event);

    void setParameters(Object ... params);
}
