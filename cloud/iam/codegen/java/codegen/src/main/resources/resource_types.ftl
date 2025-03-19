package ${package};

public final class ${class_name} {
<#list service_resources?keys as service_name>
    <#assign resources = service_resources[service_name]/>
    <#assign enum_name = resources[0].service.snakeUpperCase()/>
    public static final class ${enum_name} {
        <#list resources as resource_type>
        public static final ${global_enum_name} ${resource_type.type.snakeUpperCase()} = ${global_enum_name}.${resource_type.service.snakeUpperCase()}_${resource_type.type.snakeUpperCase()};
        </#list>
    }
</#list>
}
