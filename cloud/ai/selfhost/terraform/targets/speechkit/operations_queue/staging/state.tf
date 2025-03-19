terraform {
  backend "s3" {
    // Common part
    endpoint                    = "storage.yandexcloud.net"
    region                      = "us-east-1"
    bucket                      = "ai-terraform-state"
    skip_credentials_validation = true
    skip_metadata_api_check     = true

    // Destination key
    key = "speechkit-operations-queue-staging"
  }
}
