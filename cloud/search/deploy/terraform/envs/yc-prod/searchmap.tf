locals {
  searchmap = templatefile("../configs/searchmap.tpl", {
    backends = [
      {
        tag  = "ycsearch-backend-prod01-rc1a"
        host = "ycsearch-backend-prod01-rc1a.yandexcloud.net"
      },
      {
        tag  = "ycsearch-backend-prod01-rc1b"
        host = "ycsearch-backend-prod01-rc1b.yandexcloud.net"
      },
      {
        tag  = "ycsearch-backend-prod01-rc1c"
        host = "ycsearch-backend-prod01-rc1c.yandexcloud.net"
      },
    ]
    backends_marketplace = [
      {
        tag  = "ycsearch-mrkt-backend-prod01-rc1a"
        host = "ycsearch-mrkt-backend-prod01-rc1a.yandexcloud.net"
      },
      {
        tag  = "ycsearch-mrkt-backend-prod01-rc1b"
        host = "ycsearch-mrkt-backend-prod01-rc1b.yandexcloud.net"
      },
      {
        tag  = "ycsearch-mrkt-backend-prod01-rc1c"
        host = "ycsearch-mrkt-backend-prod01-rc1c.yandexcloud.net"
      },
    ]
    zk = "ycsearch-queue-prod01-rc1a.yandexcloud.net:8080/8081|ycsearch-queue-prod01-rc1b.yandexcloud.net:8080/8081|ycsearch-queue-prod01-rc1c.yandexcloud.net:8080/8081",
    queue_name = "yc_change_log_compute_prod",
    queue_name_marketplace = "yc_change_log_marketplace_compute_prod"
    }
  )
}
