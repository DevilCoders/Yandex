locals {
    config = {
        prod = {
            zk_endpoints              = [
                {
                    host = "zk-ml-prod-1.ru-central1.internal"
                    port = 17006
                },
                {
                    host = "zk-ml-prod-2.ru-central1.internal"
                    port = 17006
                },
                {
                    host = "zk-ml-prod-3.ru-central1.internal"
                    port = 17006
                }
            ]
            zk_global_endpoints       = [
                {
                    host = "zk-ml-prod-1.vla.ycp.cloud.yandex.net"
                    port = 17006
                },
                {
                    host = "zk-ml-prod-2.sas.ycp.cloud.yandex.net"
                    port = 17006
                },
                {
                    host = "zk-ml-prod-3.myt.ycp.cloud.yandex.net"
                    port = 17006
                }
            ]
            zk_servers                      = "zk-ml-prod-1.ru-central1.internal:17006,zk-ml-prod-2.ru-central1.internal:17006,zk-ml-prod-3.ru-central1.internal:17006"
            zk_servers_ips                  = "[2a02:6b8:c0e:500:0:f811:0:31c]:17006,[2a02:6b8:c02:900:0:f811:0:6a]:17006,[2a02:6b8:c03:500:0:f811:0:269]:17006"
            yc_endpoint                     = "api.cloud.yandex.net:443"
            common_secret                   = "sec-01dhkfv32aarnbqszm9h2cse7x"
            env_secret                      = "sec-01djn2s8h6b657he0hy6hfhpvf"
            tvm_client_id                   = "2019469"
            tvm_client_billing_id           = "2001287"
            topic_logs_common               = "b1gfcpod5hbd1ivs7dav/prod/logs-common"
            topic_logs_services_proxy       = "b1gfcpod5hbd1ivs7dav/prod/logs-services-proxy"
            topic_logs_operations_queue     = "b1gfcpod5hbd1ivs7dav/prod/logs-operations-queue"
            topic_logs_node_deployer        = "b1gfcpod5hbd1ivs7dav/prod/logs-node-deployer"
            topic_logs_node_proxy_access    = "b1gfcpod5hbd1ivs7dav/prod/logs-node-proxy-access"
            topic_billing                   = "yc.billing.service-cloud/billing-ai-requests"

            node_deployer_access_folder = "b1g4ujiu1dq5hh9544r6"
            node_deployer_creation_folder = "b1g8kqf89vea2v9p62ee"
            node_deployer_feature_flag_enabled = true
            kafka_hosts               = [
               "rc1a-utq4lc0aa0bun6po.mdb.yandexcloud.net:9092",
               "rc1b-1ssphjri7a547d33.mdb.yandexcloud.net:9092",
               "rc1c-o5d44uuaqolcnc0t.mdb.yandexcloud.net:9092"
            ]
            kafka_username = "node-deployer"
            kafka_password = "Pejn62wWBMP4VAGpuzWsdYEMgg8SHAEYFA6hkBmxUYTjRBkWwzwkVErSHNbECEj4"
            resource_pool_host = "2a0d:d6c1:0:1c::188"
            custom_models_creation_folder = "b1g8kqf89vea2v9p62ee"
            custom_models_default_subnet = "e9b7roo1cfubjhvf1j42"
            subnets                   = {
                "ru-central1-a" = "e9b7roo1cfubjhvf1j42"
                "ru-central1-b" = "e2l8g7k5ja8mjvbd3dd4"
                "ru-central1-c" = "b0crb4fds9oge0cup3en"
            }
            ds_additional_subnets    = [
              [ "e9bnvv4r6q6u53evrlih" ]
            ]
            encrypted_dek = "AAAAAQAAABRhYmpwYzVsMHQ3NmJqazE3bGI0ZwAAABBiA692OaTcP7XgzLKoC6CDAAAADCUBKjfuVMab2s4a3GB7GoAci2JZrKAabB/d2T0ACGUmRPYI6/VYPiB0pPnij+lJ9fGCuGZjWPs55yFsLUyT6BlLt14+xY3xAitnsP398m3sxDuK3wkLGjpnFlb0"
            kms_key_id = "abjthpmdh4ella5qlm86"
            op_queue_encrypted_dek = "AAAAAQAAABRhYmowMHEyMjBmcmVlajU1dmQyawAAABBXNiLJDObG9DfcpublaxlDAAAADPEi6Ou/aLuIg262Ezv89ChtyRtLTnY3mgxTtGbGQkLo2c5foIdkCESCu0tWXk8teIzfHDcd7l3GOq50oxFdG0mmEp3+GV/SkIx4zMizZuKjOU0skEM+fE5/j4eZ"
            op_queue_kms_key_id = "abjogp1mormnkalbv7ue"
            node_service_encrypted_dek = "AAAAAQAAABRhYmpzaGM5MWJpaXRmY3ZvMGJpcQAAABB5fa5wzrhNKB37Lzh/X1rPAAAADJcg14nZa8dbjpW4LBe2SU52U2obJ3FRHszywDRC/hiNM/M1y7YuCeZmXjz1aUjD4L2h8sQ7pdphO7uJ51BFQSx5eYorvwzSkUnQYH8QUur3iKmpMTNLY54N5kAJ"
            node_service_kms_key_id = "abj8nbbqaoaggactfl46"
            stt_kms_key_id = "abjeif7ckqda2c7bv6dm"
            stt_encrypted_dek = "AAAAAQAAABRhYmpuMWo0YWo4cGNnaDVoOG83YwAAABAqgWdPbswTpf68KfUIWZvVAAAADF9geQsxJ6azc4TuX69uXwbkYSJIPihIScr5H7sVGaksNFvJ/yXQyz+wbbv47vWEG0feX2jIEGwlNt4fNwhkYf7SD9kfLTu90+MW/OHd/L6LSyO/5/V0m7oM9BVC"
            tts_kms_key_id = "abjlfj142tj6kcvtes6f"
            tts_encrypted_dek = "AAAAAQAAABRhYmpqb2ZlbGw1cDNzMXFwYzhndAAAABBu3amHOGt4ymRQRaRMbvEJAAAADGL0H7P4jAI/5jNplWUxqm1b0C0Zysx+HW0G88J+UvPUda0mgr3JfLBfFhmshHCa8y1toGhWLvNfiXtPne7g7YoGa7gVS9MXzvqAfm1JyAy7GB5UQf33hj1JuONw"
            zk_kms_key_id = "abjd8l6r5eok64g96pt4"
            zk_encrypted_dek = "AAAAAQAAABRhYmplcWs4MXZhYzVqZWJhYzZsaQAAABCDCXuS4IoAd02spt7Gwh1fAAAADNtop+lDfMqUHexYUB7bmhMFtBLKqjf2HgjJyW1Gp6cL5loDwIuPcYKKU8xlx/ricCoLVlIhc1jVssHwPjvuZAzCIR4O1kD6Flt0Ico6ISKeuCi9TdaM4SEK9sAr"
            solomon_cluster = "cloud_ai_prod"
        }
        preprod = {
            zk_endpoints              = [
                {
                    host = "zk-ml-preprod-1.ru-central1.internal"
                    port = 17006
                },
                {
                    host = "zk-ml-preprod-2.ru-central1.internal"
                    port = 17006
                },
                {
                    host = "zk-ml-preprod-3.ru-central1.internal"
                    port = 17006
                }
            ]
            zk_global_endpoints          = [
                {
                    host = "zookeeper-staging-1.datasphere.cloud.yandex.net"
                    port = 17006
                },
                {
                    host = "zookeeper-staging-2.datasphere.cloud.yandex.net"
                    port = 17006
                },
                {
                    host = "zookeeper-staging-3.datasphere.cloud.yandex.net"
                    port = 17006
                }
            ]
            zk_servers                      = "zk-ml-preprod-1.ru-central1.internal:17006,zk-ml-preprod-2.ru-central1.internal:17006,zk-ml-preprod-3.ru-central1.internal:17006"
            zk_servers_ips                  = "[2a02:6b8:c0e:500:0:f816:0:3da]:17006,[2a02:6b8:c02:900:0:f816:0:100]:17006,[2a02:6b8:c03:500:0:f816:0:2a]:17006"
            yc_endpoint                     = "api.cloud.yandex.net:443"
            common_secret                   = "sec-01dhkfv32aarnbqszm9h2cse7x"
            env_secret                      = "sec-01djmfwpjn2a5hwzn7gcm2mmcz"
            tvm_client_id                   = "2019471"
            tvm_client_billing_id           = "2001289"
            topic_logs_common               = "b1gfcpod5hbd1ivs7dav/preprod/logs-common"
            topic_logs_services_proxy       = "b1gfcpod5hbd1ivs7dav/preprod/logs-services-proxy"
            topic_logs_operations_queue     = "b1gfcpod5hbd1ivs7dav/preprod/logs-operations-queue"
            topic_logs_node_deployer        = "b1gfcpod5hbd1ivs7dav/preprod/logs-node-deployer"
            topic_logs_node_proxy_access    = "b1gfcpod5hbd1ivs7dav/preprod/logs-node-proxy-access"
            topic_billing                   = "yc.billing.service-cloud/billing-ai-requests-test"

            node_deployer_access_folder     = "b1g19hobememv3hj6qsc"
            node_deployer_creation_folder   = "b1g9ugnkudt9t1ohgvma"
            node_deployer_feature_flag_enabled = false
            kafka_hosts               = [
               "rc1a-j5jo8tcvpfpj3gkr.mdb.yandexcloud.net:9092",
               "rc1b-dtfnqcgn5n2jofte.mdb.yandexcloud.net:9092",
               "rc1c-g8tdd37ur7qeocll.mdb.yandexcloud.net:9092"
            ]
            kafka_username = "node-deployer"
            kafka_password = "Pejn62wWBMP4VAGpuzWsdYEMgg8SHAEYFA6hkBmxUYTjRBkWwzwkVErSHNbECEj4"
            resource_pool_host = "2a0d:d6c1:0:1c::3cf"
            custom_models_creation_folder = "b1g9ugnkudt9t1ohgvma"
            custom_models_default_subnet = "e9brt77u0kenestd9232"
            subnets                   = {
                "ru-central1-a" = "e9brt77u0kenestd9232"
                "ru-central1-b" = "e2lngl3r1nbqiijbnck5"
                "ru-central1-c" = "b0cmpfls1vav7l48g2nn"
            }
            ds_additional_subnets    = [
              [ "e9buqjf2tl559g0u5grt" ]
            ]
            encrypted_dek = "AAAAAQAAABRhYmo2NHNlZWVnOXFncXJjbnEyNwAAABDevRXSnw6HdwYGYvJxlXxcAAAADAw3E/cPoBBOXPMM9rVTeQdLsMwwUL5nNaV76Gq7XkiRy484q8EOog/92002O4Mxx68E90VeqCFR/ZSyTcQIneIB4WHDC2PyNW6emweNwihnL1W4VMAIwbxYTyJj"
            kms_key_id = "abjldof6gp0ktubfkbup"
            op_queue_encrypted_dek = "AAAAAQAAABRhYmpmdDNsaDFhczhuNWcxMW43dAAAABBK3g8k5ZZK0RSVV4GU+I7eAAAADM7jFU4NXw/C1SFrbwXtuZwLBhR6tGT2n+6IyEJ5Pd0BqUUQX8Rzcx7DDsXQ5kwz63kU5WgPNb7RlRMNYfitqZ23cYHv1K7mCJddMABXNWRv0vFY9ikY5byG7Col"
            op_queue_kms_key_id = "abjvufv4p1p0dqk6a474"
            node_service_encrypted_dek = "AAAAAQAAABRhYmp1N2dvc2o1aDU5MTBwNnExYgAAABBnVrujOrN5/kOvG4aV4ZjEAAAADFijOyG1P+dQHKJzFw9yxRk4iSjDT2ljtjAcSKNBwBM/ZP6N6adEU2tiWJ94OwBR9zv2eK8zSueQYuAZxSLCPi6EdX5T/t9RjX64VRxCnnL28q3wHtOjlV5kJ/fQ"
            node_service_kms_key_id = "abj8ghidd3nfqm19gs1j"
            stt_kms_key_id = "abjjnrtdjfnt8snk72mq"
            stt_encrypted_dek = "AAAAAQAAABRhYmozMDIxMmc3ZDc4bGkxOHExMAAAABCAxyXU08RW7V594a3LB+1HAAAADNsTObASsdz+s9w2SsqloeEQuzh+df2kQQ2cN14DQxNYMgb5D4cJD2wavt7QViGDatbmLtk7ff+cvhb28q7GWWWKFmvvEwW/jgcAK6U5ssyIPkY4kgXdYSE+ZT3U"
            tts_kms_key_id = "abjdpsbn6im2n4neljb5"
            tts_encrypted_dek = "AAAAAQAAABRhYmp0Ymd2ZGIxYWo1bWx2NGJmbQAAABDhet1lntDKTgubqL15Hj0GAAAADBRh7UlgPJ3823kghEK41IJqav2jZi+bdqg33900dMqXnzF0dIAUTE+IYNf+gTU2nyesAQU9Nm5tU/BJO3yoqtu3bq6j+fSIsco9i8unhumaj4FS5C15OY57R18D"
            zk_kms_key_id = "abjqcqt67o1ec5t5frf5"
            zk_encrypted_dek = "AAAAAQAAABRhYmpzMzc4ZDR2Y2hkdWVtN3RjNwAAABA07aon/QfaMol+JuQSvECvAAAADBArwuquFk3pteF6cQbfI6+fwMDU+Uv8TBcwq8sXvCc2HINwkNF3m3MctRS9mgt0fS3a7vcZPVY1biS7fWHk4F9QdcmMx7txJWYDR41A4Q/Tj4MdGqCE2dfI6nax"
            solomon_cluster = "cloud_ai_test"
        }
    }
}

output "by_environment" {
  value = lookup(local.config, var.environment, local.config.preprod)
}
