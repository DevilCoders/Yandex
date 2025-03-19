locals {
  services = {
    yc_search_proxy = {
      service_name   = "yc_search_proxy"
      packet_name    = "yc-search-proxy"
      packet_version = "0.9151479.trunk"
      image_id       = "fdvtmaqm2mjf9vafpdbk" # yc --profile search-preprod compute image get-latest-from-family yc-search-base-1804
    }

    yc_search_backend = {
      service_name   = "yc_search_backend"
      packet_name    = "yc-search-backend"
      packet_version = "0.9151479.trunk"
      image_id       = "fdvtmaqm2mjf9vafpdbk" # yc --profile search-preprod compute image get-latest-from-family yc-search-base-1804
    }

    yc_search_marketplace_backend = {
      service_name   = "yc_search_backend"
      packet_name    = "yc-search-backend"
      packet_version = "0.9151479.trunk"
      image_id       = "fdvtmaqm2mjf9vafpdbk" # yc --profile search-preprod compute image get-latest-from-family yc-search-base-1804
    }

    yc_search_queue = {
      service_name   = "yc_search_queue"
      packet_name    = "yc-search-queue"
      packet_version = "0.8743723.trunk"
      image_id       = "fdvtmaqm2mjf9vafpdbk" # yc --profile search-preprod compute image get-latest-from-family yc-search-base-1804
    }

    yc_search_indexer = {
      service_name   = "yc_search_indexer"
      packet_name    = "yc-search-indexer"
      packet_version = "0.9151479.trunk"
      image_id       = "fdvtmaqm2mjf9vafpdbk" # yc --profile search-preprod compute image get-latest-from-family yc-search-base-1804
    }
  }
}
