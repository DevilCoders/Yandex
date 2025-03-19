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
        "bucpba0hulgrkgpd58qp",
        "bltueujt22oqg5fod2se",
        "fo27jfhs8sfn4u51ak2s",
    ]

    domain = "ycp.cloud-preprod.yandex.net"

    environment = "preprod"

    abc_svc = "yckubernetes"
}