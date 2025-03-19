package ${package};

public final class ${class_name} {
<#list service_quotas?keys as service_name>
    <#assign quotas = service_quotas[service_name]/>
    <#assign enum_name = quotas[0].service.snakeUpperCase()/>
    public static final class ${enum_name} {
        <#list quotas as quota>
        public static final ${global_enum_name} ${quota.type.snakeUpperCase()} = ${global_enum_name}.${quota.service.snakeUpperCase()}_${quota.type.snakeUpperCase()};
        </#list>
    }
</#list>
}
