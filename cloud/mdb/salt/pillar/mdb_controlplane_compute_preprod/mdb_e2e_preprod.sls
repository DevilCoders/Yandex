mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

include:
    - envs.qa
    - mdb_controlplane_compute_preprod.common
    - compute.preprod.e2e

data:
    deploy:
        version: 2
        api_host: mdb-deploy-api.private-api.cloud-preprod.yandex.net
    cauth_use: False
    ipv6selfdns: True
    monrun2: True
    l3host: True
    dbaas:
        vtype: compute
    dbaas-e2e:
        api_url: https://mdb.private-api.cloud-preprod.yandex.net
        grpc_api_url: mdb-internal-api.private-api.cloud-preprod.yandex.net:443
        test_hadoop: True
        test_hadoop_lightweight: True
        test_elasticsearch: True
        test_clickhouse_cloud_storage: True
        test_sqlserver: True
        test_kafka: True
        test_greenplum: True
        test_cases:
            compute-preprod:
                folder_id: aoed5i52uquf5jio0oec
                network_id: c64qnhdg1rem34d2fpl9
                subnet_id: fo2el9pq5gh6cns3957b
                dualstack_network_id: c64vs98keiqc7f24pvkd
                dualstack_subnet_id: fo2efq4nd4sac5m85tnt
                log_file: /var/log/monrun/dbaas-e2e-compute-preprod.log
                environment: PRODUCTION
                dbname: dbaas_e2e_compute_preprod
                api_client: internal
                iam:
                    host: ts.private-api.cloud-preprod.yandex.net:4282
                    cert_file: /opt/yandex/allCAs.pem
                sa_creds:
                    service_account_id: bfb1ifpmtbgv8aca7j3b
                    id: bfbnjddves4a0lf9328f
                    private_key: |
                        {{ salt.yav.get('ver-01ek2e5tgg2yqhn5hxg2hkncft[private_key]') | indent(24) }}
                flavor: s2.micro
                redis_flavor: hm2.nano
                sqlserver_flavor: s2.small
                elasticsearch_flavor: s2.micro
                geo_map:
                    myt: ru-central1-c
                    sas: ru-central1-b
                    vla: ru-central1-a
                    man: ru-central1-c
                s3_bucket: dataproc-e2e
                hadoop_flavor: s2.micro
                hadoop_log_group_id: af37ejm1mb0iggudg2v5
                greenplum_segment_flavor: s2.micro
                greenplum_master_flavor: s2.micro
    selfdns-api:
        plugins-enabled:
            - ipv6only
    control_plane_interfaces: False

    runlist:
        - components.dbaas-e2e
        - components.dbaas-compute-controlplane
        - components.dbaas-compute.network

cert.ca: |
    -----BEGIN CERTIFICATE-----
    MIIFZTCCA02gAwIBAgIKUlD06gAAAAAAGDANBgkqhkiG9w0BAQ0FADAfMR0wGwYD
    VQQDExRZYW5kZXhJbnRlcm5hbFJvb3RDQTAeFw0xODA2MjgxMTE0NTdaFw0zMjA2
    MjgxMTI0NTdaMFsxEjAQBgoJkiaJk/IsZAEZFgJydTEWMBQGCgmSJomT8ixkARkW
    BnlhbmRleDESMBAGCgmSJomT8ixkARkWAmxkMRkwFwYDVQQDExBZYW5kZXhJbnRl
    cm5hbENBMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAy6Sab1PCbISk
    GSAUpr6JJKLXlf4O+cBhjALfQn2QpPL/cDjZ2+MPXuAUgE8KT+/mbAGA2rJID0KY
    RjDSkByxnhoX8jwWsmPYXoAmOMPkgKRG9/ZefnMrK4oVhGgLmxnpbEkNbGh88cJ1
    OVzgD5LVHSpDqm7iEuoUPOJCWXQ51+rZ0Lw9zBEU8v3yXXI345iWpLj92pOQDH0G
    Tqr7BnQywxcgb5BYdywayacIT7UTJZk7832m5k7Oa3qMIKKXHsx26rNVUVBfpzph
    OFvqkLetOKHk7827NDKr3I3OFXzQk4gy6tagv8PZNp+XGOBWfYkbLfI4xbTnjHIW
    n5q1gfKPOQIDAQABo4IBZTCCAWEwEAYJKwYBBAGCNxUBBAMCAQIwIwYJKwYBBAGC
    NxUCBBYEFNgaef9LcdQKs6qfsfiuWF5p/yqRMB0GA1UdDgQWBBSP3TKDCRNT3ZEa
    Zumz1DzFtPJnSDBZBgNVHSAEUjBQME4GBFUdIAAwRjBEBggrBgEFBQcCARY4aHR0
    cDovL2NybHMueWFuZGV4LnJ1L2Nwcy9ZYW5kZXhJbnRlcm5hbENBL3BvbGljaWVz
    Lmh0bWwwGQYJKwYBBAGCNxQCBAweCgBTAHUAYgBDAEEwCwYDVR0PBAQDAgGGMA8G
    A1UdEwEB/wQFMAMBAf8wHwYDVR0jBBgwFoAUq7nF/6Hv5lMdMzkihNF21DdOLWow
    VAYDVR0fBE0wSzBJoEegRYZDaHR0cDovL2NybHMueWFuZGV4LnJ1L1lhbmRleElu
    dGVybmFsUm9vdENBL1lhbmRleEludGVybmFsUm9vdENBLmNybDANBgkqhkiG9w0B
    AQ0FAAOCAgEAQnOiyykjwtSuCBV6rSiM8Q1rQIcfyqn1JBxSGeBMABc64loWSPaQ
    DtYPIW5rwNX7TQ94bjyYgCxhwHqUED/fcBOmXCQ2iBsdy5LOcNEZaC2kBHQuZ7dL
    0fSvpE98a41y9yY6CJGFXg8E/4GrQwgQEqT5Qbe9GHPadpRu+ptVvI6uLZG3ks2o
    oodjOm5C0SIo1pY4OtPAYE/AzTaYkTFbAqYcPfEfXHEOigBJBeXnQs7cANxX/RaF
    PnHEjZbGY57EtBP6p5ckndkfEmqp3PLXbsQteNOVpsUw5eVqEzinSisBmLc28nnr
    5QEojRontAaZd7ZzB5zaGkVuE+0laUUWSNBhfGE1R3LrTJEK9L7FEsBBprOxIWww
    CvLmAfglouwuNRc2TjRdfnZaEfPLD7NYIF4ahXPAMcfTii23Tlr2uB7LetNykSlX
    Z9S5/yf61VFEKnxuipFPNgtKqPcFgFUxlEb+wOeOfYZ7ex8VlpMBWbadj3Go025b
    KZUwKwHDQvgJ5pz9g3t+t5Xieu2pwyddWGu+1SItRohRhlyTiep7oW6yTps7Qt0e
    8pdLuLG7ZF19h1Pxi+dVbeaeNcsGEAOdRuCk+RTZHNe+J4yC8tNJOepnfYDul6SB
    RjFWthiFK45+TZRHAcsG9JuV8JNvgoKaL75v/GUsKaeJ3Cps3rBStfc=
    -----END CERTIFICATE-----
    -----BEGIN CERTIFICATE-----
    MIIFGTCCAwGgAwIBAgIQJMM7ZIy2SYxCBgK7WcFwnjANBgkqhkiG9w0BAQ0FADAf
    MR0wGwYDVQQDExRZYW5kZXhJbnRlcm5hbFJvb3RDQTAeFw0xMzAyMTExMzQxNDNa
    Fw0zMzAyMTExMzUxNDJaMB8xHTAbBgNVBAMTFFlhbmRleEludGVybmFsUm9vdENB
    MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAgb4xoQjBQ7oEFk8EHVGy
    1pDEmPWw0Wgw5nX9RM7LL2xQWyUuEq+Lf9Dgh+O725aZ9+SO2oEs47DHHt81/fne
    5N6xOftRrCpy8hGtUR/A3bvjnQgjs+zdXvcO9cTuuzzPTFSts/iZATZsAruiepMx
    SGj9S1fGwvYws/yiXWNoNBz4Tu1Tlp0g+5fp/ADjnxc6DqNk6w01mJRDbx+6rlBO
    aIH2tQmJXDVoFdrhmBK9qOfjxWlIYGy83TnrvdXwi5mKTMtpEREMgyNLX75UjpvO
    NkZgBvEXPQq+g91wBGsWIE2sYlguXiBniQgAJOyRuSdTxcJoG8tZkLDPRi5RouWY
    gxXr13edn1TRDGco2hkdtSUBlajBMSvAq+H0hkslzWD/R+BXkn9dh0/DFnxVt4XU
    5JbFyd/sKV/rF4Vygfw9ssh1ZIWdqkfZ2QXOZ2gH4AEeoN/9vEfUPwqPVzL0XEZK
    r4s2WjU9mE5tHrVsQOZ80wnvYHYi2JHbl0hr5ghs4RIyJwx6LEEnj2tzMFec4f7o
    dQeSsZpgRJmpvpAfRTxhIRjZBrKxnMytedAkUPguBQwjVCn7+EaKiJfpu42JG8Mm
    +/dHi+Q9Tc+0tX5pKOIpQMlMxMHw8MfPmUjC3AAd9lsmCtuybYoeN2IRdbzzchJ8
    l1ZuoI3gH7pcIeElfVSqSBkCAwEAAaNRME8wCwYDVR0PBAQDAgGGMA8GA1UdEwEB
    /wQFMAMBAf8wHQYDVR0OBBYEFKu5xf+h7+ZTHTM5IoTRdtQ3Ti1qMBAGCSsGAQQB
    gjcVAQQDAgEAMA0GCSqGSIb3DQEBDQUAA4ICAQAVpyJ1qLjqRLC34F1UXkC3vxpO
    nV6WgzpzA+DUNog4Y6RhTnh0Bsir+I+FTl0zFCm7JpT/3NP9VjfEitMkHehmHhQK
    c7cIBZSF62K477OTvLz+9ku2O/bGTtYv9fAvR4BmzFfyPDoAKOjJSghD1p/7El+1
    eSjvcUBzLnBUtxO/iYXRNo7B3+1qo4F5Hz7rPRLI0UWW/0UAfVCO2fFtyF6C1iEY
    /q0Ldbf3YIaMkf2WgGhnX9yH/8OiIij2r0LVNHS811apyycjep8y/NkG4q1Z9jEi
    VEX3P6NEL8dWtXQlvlNGMcfDT3lmB+tS32CPEUwce/Ble646rukbERRwFfxXojpf
    C6ium+LtJc7qnK6ygnYF4D6mz4H+3WaxJd1S1hGQxOb/3WVw63tZFnN62F6/nc5g
    6T44Yb7ND6y3nVcygLpbQsws6HsjX65CoSjrrPn0YhKxNBscF7M7tLTW/5LK9uhk
    yjRCkJ0YagpeLxfV1l1ZJZaTPZvY9+ylHnWHhzlq0FzcrooSSsp4i44DB2K7O2ID
    87leymZkKUY6PMDa4GkDJx0dG4UXDhRETMf+NkYgtLJ+UIzMNskwVDcxO4kVL+Hi
    Pj78bnC5yCw8P5YylR45LdxLzLO68unoXOyFz1etGXzszw8lJI9LNubYxk77mK8H
    LpuQKbSbIERsmR+QqQ==
    -----END CERTIFICATE-----
