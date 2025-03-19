package ${package};

public enum ${enum_name} {
<#list restrictionTypes as restrictionType>
    ${restrictionType.kind.snakeUpperCase()}_${restrictionType.type.snakeUpperCase()}(<#rt>
        "${restrictionType.name}", <#t>
        ${kind_enum_name}.${restrictionType.kind.snakeUpperCase()}<#t>
    )<#if restrictionType?has_next>,<#else>;</#if><#lt>
</#list>

    private final String value;
    private final ${kind_enum_name} kind;

    ${enum_name}(String value, ${kind_enum_name} kind) {
        this.value = value;
        this.kind = kind;
    }

    public String getValue() {
        return value;
    }

    public ${kind_enum_name} getKind() {
        return kind;
    }

    @Override
    public String toString() {
        return value;
    }
}
