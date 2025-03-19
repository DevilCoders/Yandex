package ${package};

public enum ${enum_name} {
<#list resources as resource_type>
    ${resource_type.service.snakeUpperCase()}_${resource_type.type.snakeUpperCase()}(<#rt>
        "${resource_type.name}", <#t>
        Service.${services[resource_type.serviceName].service.snakeUpperCase()}<#t>
    )<#if resource_type?has_next>,<#else>;</#if><#lt>
</#list>

    private String value;
    private Service service;

    ${enum_name}(String value, Service service) {
        this.value = value;
        this.service = service;
    }

    public String getValue() {
        return value;
    }

    public Service getService() {
        return service;
    }

    @Override
    public String toString() {
        return value;
    }
}
