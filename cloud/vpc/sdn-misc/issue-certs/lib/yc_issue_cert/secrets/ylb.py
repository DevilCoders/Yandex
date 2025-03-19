from yc_issue_cert.secrets import SecretFactory, ServicePemCertificate, BaseRole


@SecretFactory.register("hc-ctrl-server-pem")
class YlbHCApiCertificate(ServicePemCertificate):
    USER, GROUP = "yc-healthcheck-ctrl", "yc-healthcheck-ctrl"
    PATH = "/etc/yc/healthcheck-ctrl/hc.private-api.pem"


@SecretFactory.register("lb-ctrl-server-pem")
class YlbLBApiCertificate(ServicePemCertificate):
    USER, GROUP = "yc-loadbalancer-ctrl", "yc-loadbalancer-ctrl"
    PATH = "/etc/yc/loadbalancer-ctrl/lb.private-api.pem"


@SecretFactory.register("ylb-host-cert-lb-node-pem")
class YlbHostTlsLbNodeCertificate(ServicePemCertificate):
    USER, GROUP = "yc-loadbalancer-node", "yc-loadbalancer-node"
    USE_SCOPE = True
    BASE_ROLES = [BaseRole.LB_NODE]
    PATH = "/etc/yc/loadbalancer-node/cert.pem"


@SecretFactory.register("ylb-host-cert-lb-ctrl-pem")
class YlbHostTlsLbCtrlCertificate(ServicePemCertificate):
    USER, GROUP = "yc-loadbalancer-ctrl", "yc-loadbalancer-ctrl"
    USE_SCOPE = True
    BASE_ROLES = [BaseRole.LB_CTRL]
    PATH = "/etc/yc/loadbalancer-ctrl/cert.pem"


@SecretFactory.register("ylb-host-cert-hc-node-pem")
class YlbHostTlsHcNodeCertificate(ServicePemCertificate):
    USER, GROUP = "yc-healthcheck-node", "yc-healthcheck-node"
    USE_SCOPE = True
    BASE_ROLES = [BaseRole.HC_NODE]
    PATH = "/etc/yc/healthcheck-node/cert.pem"


@SecretFactory.register("ylb-host-cert-hc-ctrl-pem")
class YlbHostTlsHcCtrlCertificate(ServicePemCertificate):
    USER, GROUP = "yc-healthcheck-ctrl", "yc-healthcheck-ctrl"
    USE_SCOPE = True
    BASE_ROLES = [BaseRole.HC_CTRL]
    PATH = "/etc/yc/healthcheck-ctrl/cert.pem"
