locals {
  searchmap = templatefile("../configs/searchmap.tpl", {
    backends = [
      {
        tag  = "ycsearch-backend-preprod01-rc1a"
        host = "ycsearch-backend-preprod01-rc1a.cloud-preprod.yandex.net"
      },
      {
        tag  = "ycsearch-backend-preprod01-rc1b"
        host = "ycsearch-backend-preprod01-rc1b.cloud-preprod.yandex.net"
      },
      {
        tag  = "ycsearch-backend-preprod01-rc1c"
        host = "ycsearch-backend-preprod01-rc1c.cloud-preprod.yandex.net"
      },
    ]
    backends_marketplace = [
      {
        tag  = "ycsearch-mrkt-backend-preprod01-rc1a"
        host = "ycsearch-mrkt-backend-preprod01-rc1a.cloud-preprod.yandex.net"
      },
      {
        tag  = "ycsearch-mrkt-backend-preprod01-rc1b"
        host = "ycsearch-mrkt-backend-preprod01-rc1b.cloud-preprod.yandex.net"
      },
      {
        tag  = "ycsearch-mrkt-backend-preprod01-rc1c"
        host = "ycsearch-mrkt-backend-preprod01-rc1c.cloud-preprod.yandex.net"
      },
    ]
    zk = "ycsearch-queue-preprod01-rc1a.cloud-preprod.yandex.net:8080/8081|ycsearch-queue-preprod01-rc1b.cloud-preprod.yandex.net:8080/8081|ycsearch-queue-preprod01-rc1c.cloud-preprod.yandex.net:8080/8081",
    queue_name = "yc_change_log_compute_preprod"
    queue_name_marketplace = "yc_change_log_marketplace_compute_preprod"
    }
  )
}
