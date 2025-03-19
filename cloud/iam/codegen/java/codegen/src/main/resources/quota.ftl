package ${package};

public enum ${enum_name} {
<#list quotas as quota>
    ${quota.service.snakeUpperCase()}_${quota.type.snakeUpperCase()}(<#rt>
        "${quota.name}", <#t>
        Service.${services[quota.serviceName].service.snakeUpperCase()}<#t>
    )<#if quota?has_next>,<#else>;</#if><#lt>
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
