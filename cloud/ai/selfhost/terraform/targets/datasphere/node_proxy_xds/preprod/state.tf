terraform {
  backend "s3" {
    // Common part
    endpoint                    = "storage.yandexcloud.net"
    region                      = "us-east-1"
    bucket                      = "ai-terraform-state"
    skip_credentials_validation = true
    skip_metadata_api_check     = true

    // Destination key
    // TODO: Rename key to node-proxy-xds-service-preprod
    key = "node_proxy_xds_service_preprod"
  }
}
