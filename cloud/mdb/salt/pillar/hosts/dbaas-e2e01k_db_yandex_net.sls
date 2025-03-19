mine_functions:
    grains.item:
        - id
        - role
        - ya
        - pg
        - virtual

include:
    - envs.dev
    - porto.prod.selfdns.realm-sandbox
    - porto.prod.dbaas.e2e

data:
    monrun2: True
    dbaas-e2e:
        api_url: https://internal-api-test.db.yandex-team.ru
        grpc_api_url: mdb-internal-api-test.db.yandex.net:443
        test_elasticsearch: True
        test_clickhouse_cloud_storage: True
        test_kafka: True
        test_greenplum: True
        test_cases:
            porto-qa:
                vtype: porto
                folder_id: 'foorv7rnqd9sfo4q6db4'
                network_id: ''
                iam:
                    host: ts.cloud.yandex-team.ru:4282
                    cert_file: /opt/yandex/allCAs.pem
                sa_creds:
                    service_account_id: f6oc1ukj6fn9uvlesuoq
                    id: f6oq85kqf2ln6knfngft
                    private_key: |
                        {{ salt.yav.get('ver-01ekyezca8hd5j121my491gfdm[private_key]') | indent(24) }}
                flavor: s2.nano
                environment: PRODUCTION
                log_file: /var/log/monrun/dbaas-e2e-porto-qa.log
                dbname: dbaas_e2e_porto_qa
                api_client: internal
                cloud_org_id: '58515cc5-187b-4b73-a8d2-871955163cce'
                disk_type: "local-ssd"
                redis_flavor: m2.nano
                geo_map:
                    man: iva
                    vla: myt
                greenplum_segment_flavor: s2.micro
                greenplum_master_flavor: s2.micro

    runlist:
        - components.dbaas-e2e

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
