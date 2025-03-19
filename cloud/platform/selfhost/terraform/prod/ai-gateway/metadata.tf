locals {
    metadata_files = {
        # metadata only
        k8s-runtime-bootstrap-yaml = file("${path.module}/files/bootstrap.yaml")
        skm = file("${path.module}/files/skm.yaml")
        user-data = file("${path.module}/files/user-data.yaml")

        # Files
        als-config = file("${path.module}/files/configs/als.yaml")
        configserver-config = file("${path.module}/files/configs/configserver.yaml")
        envoy-config = file("${path.module}/files/configs/envoy.yaml")
        envoy-resources-config = file("${path.module}/files/configs/envoy-resources.yaml")
        metricsagent-config = file("${path.module}/files/metricsagent.yaml")
        push-client-config = file("${path.module}/files/push-client.yaml")
        jaeger-config-yaml = file("${path.module}/files/jaeger-config.yaml")
    }
    metadata_files_blue = merge(local.metadata_files, {
        gateway-config = file("${path.module}/files/configs/gateway.blue.yaml")
        gateway-services-config = file("${path.module}/files/configs/gateway-services.yaml")
    })
    metadata_files_green = merge(local.metadata_files, {
        gateway-config = file("${path.module}/files/configs/gateway.green.yaml")
        gateway-services-config = file("${path.module}/files/configs/gateway-services.yaml")
    })
}
