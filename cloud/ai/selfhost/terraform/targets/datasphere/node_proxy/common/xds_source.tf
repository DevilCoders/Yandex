locals {

    /*
     * TODO: Remove this hardcode
     *       Currenlty public yandex cloud provider does not expose ipv6 for instances
     *       and private provider does not expose instatnces at all
     *       It's hard to provide constistency with this naming in the dns
     * WARN: Please verify naming before each deploy
     */

    preprod_xds_endpoint_by_zone = {
        ru-central1-a = "node-proxy-xds-preprod-1.datasphere.cloud-preprod.yandex.net"
        ru-central1-b = "node-proxy-xds-preprod-2.datasphere.cloud-preprod.yandex.net"
        ru-central1-c = "node-proxy-xds-preprod-3.datasphere.cloud-preprod.yandex.net"
    }

    staging_xds_endpoint_by_zone = {
        ru-central1-a = "node-proxy-xds-staging-1.datasphere.cloud.yandex.net"
        ru-central1-b = "node-proxy-xds-staging-2.datasphere.cloud.yandex.net"
        ru-central1-c = "node-proxy-xds-staging-3.datasphere.cloud.yandex.net"
    }

    prod_xds_endpoint_by_zone = {
        ru-central1-a = "node-proxy-xds-prod-1.datasphere.cloud.yandex.net"
        ru-central1-b = "node-proxy-xds-prod-2.datasphere.cloud.yandex.net"
        ru-central1-c = "node-proxy-xds-prod-3.datasphere.cloud.yandex.net"
    }

    xds_endpoint_by_zone_by_environment = {
        preprod = local.preprod_xds_endpoint_by_zone,
        staging = local.staging_xds_endpoint_by_zone,
        prod = local.prod_xds_endpoint_by_zone
    }

    xds_endpoints_by_zone = local.xds_endpoint_by_zone_by_environment[var.environment]
}