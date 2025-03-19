terraform {
  backend "s3" {
    endpoint   = "storage.yandexcloud.net"
    bucket     = "ai-terraform-state"
    key        = "app-stt-fio"
    region     = "us-east-1"
    // Access is done via the terraform-maintainer service account from cloud_ai common folder.
    // Use ./scripts/terraform-init.sh for init
    access_key = "fHNdo3JRxGBaPymGbqZd"

    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}
