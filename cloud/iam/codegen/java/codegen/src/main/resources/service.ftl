package ${package};

import java.util.List;

public enum ${enum_name} {
<#list services as service>
    ${service.service.snakeUpperCase()}("${service.serviceName}", List.of(<#rt>
        <#list service.aliases as alias>
            "${alias}"<#if alias?has_next>, </#if><#t>
        </#list>
    ))<#if service?has_next>,<#else>;</#if><#lt>
</#list>

    private String value;
    private List<String> aliases;

    ${enum_name}(String value, List<String> aliases) {
        this.value = value;
        this.aliases = aliases;
    }

    public String getValue() {
        return value;
    }

    public List<String> getAliases() {
        return aliases;
    }

    @Override
    public String toString() {
        return value;
    }
}
