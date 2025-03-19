package ${package};

public enum ${enum_name} {
<#list restriction_kinds as restriction_kind>
    ${restriction_kind.restrictionKind.snakeUpperCase()}(<#rt>
        "${restriction_kind.name}"<#t>
    )<#if restriction_kind?has_next>,<#else>;</#if><#lt>
</#list>

    private final String value;

    ${enum_name}(String value) {
        this.value = value;
    }

    public String getValue() {
        return value;
    }

    @Override
    public String toString() {
        return value;
    }
}
