locals {
    datacenters = [
        "vla",
        "sas",
        "myt",
    ]

    zones = [
        "ru-central1-a",
        "ru-central1-b",
        "ru-central1-c",
    ]

    subnets = [
        "ema43t9j4dhhlblb6hjv",
        "fkp3latk8pme0pfuujom",
        "flqmhdvdda2vrrc2glkr",
    ]

    domain = "ycp.cloud-testing.yandex.net"

    environment = "testing"

    abc_svc = "ycl7"

    zone_regions = {
        "ru-central1-a" = "ru-central1",
        "ru-central1-b" = "ru-central1",
        "ru-central1-c" = "ru-central1",
    }
}