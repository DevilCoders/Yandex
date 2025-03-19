package ${package};
public final class ${class_name} {
<#list permissions as permission>
    public static final String <#if permission.serviceName != "without-service">${permission.service.snakeUpperCase()}_</#if>${permission.type.snakeUpperCase()} = "${permission.name}";
</#list>
}
