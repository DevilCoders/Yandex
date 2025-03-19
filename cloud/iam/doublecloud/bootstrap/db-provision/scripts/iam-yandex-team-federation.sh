#!/bin/bash
set -e

ORG_URL=${1}

if [[ -z ${ORG_URL} ]]; then
    echo "usage: $0 [ORG_URL]"
    ORG_URL=org.private-api.eu-central-1.aws.datacloud.net
    TS_URL=ts.private-api.eu-central-1.aws.datacloud.net
else
    TS_URL=${ORG_URL?}
fi

IAM_TOKEN=$(internal/get-eve-token.sh ${TS_URL})

## test federation_id fsf226a30ui43du3l93u
#./internal/create-federation.sh "${IAM_TOKEN?}" "fsf226a30ui43du3l93u" "yc.organization-manager.yandex" "yandex-team-federation-test" "TEST Yandex team federation" "https://keycloak-test.cloud.yandex.net:8443/auth/realms/master" "https://keycloak-test.cloud.yandex.net:8443/auth/realms/master/protocol/saml" ${ORG_URL?}
#./internal/create-federation-cert.sh "${IAM_TOKEN?}" \
#        "fsf226a30ui43du3l93u" \
#        "cert" \
#        "-----BEGIN CERTIFICATE-----\nMIIDjzCCAnegAwIBAgIUF3bSIPKEcz0+93czW8h814WWGl8wDQYJKoZIhvcNAQELBQAwVzELMAkGA1UEBhMCUlUxDzANBgNVBAgMBk1vc2NvdzEPMA0GA1UECgwGWWFuZGV4MRUwEwYDVQQLDAxZYW5kZXguQ2xvdWQxDzANBgNVBAMMBllhbmRleDAeFw0yMDAyMTIwODI2NTVaFw0yNTAyMTIwODI2NTVaMFcxCzAJBgNVBAYTAlJVMQ8wDQYDVQQIDAZNb3Njb3cxDzANBgNVBAoMBllhbmRleDEVMBMGA1UECwwMWWFuZGV4LkNsb3VkMQ8wDQYDVQQDDAZZYW5kZXgwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDYVcxqC0Ienc85C2oD93GvYB172VSPYuy7umjHa/8Xy8KUGmAjO1pJA2Qvgrs0AmymueM6d6Uw0C7MsDY/Z3eOid9ZRnbmJtfx2MzmyA91Y0ZvcIxYXWa4kloEiLUMGs94ixgBat+erKN836NVF4mFOtLUsueMOkQkdFw6RPlw9NccEmWxb/XfzA2fceCjWFeJt1RQSAMPxmsd+s9NmsaZ7dFsn1Vx/ACLdlzxhyCjf7A5FIRZUxEHQF5UOP7z2bKkErDivFj19XhZ2whHmHNKy2pvrzZ0ufoy2+isW5HzEn1+DO3hwX8pKOOvjtKi5vqtsdjteGDmF2+lm+RxrkL/AgMBAAGjUzBRMB0GA1UdDgQWBBRVWZqkoCfZHnN6vDN6d5l2tQGp7jAfBgNVHSMEGDAWgBRVWZqkoCfZHnN6vDN6d5l2tQGp7jAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQA4/gJ1/dawiBYa4hYpCFsnWIgoJ9iLgG6mOZ6q0xT8X/zAPkA9EeB75OdFPFruAydRlihdiVfrjbdOYoEgrTbF1jtWAAK+YQpLohrLp070h5PzXMMvso8Tkg4phxLEGhtsHg+SMFipvg0uTG1CWttgnHy8tWfrjbrF2MPA846ibHu7nPjhwoQxdd5kILPYLTANc3YzwaiN96f6flZgedkVDUHMT2AVMpFvbYb1jNqULQ9Xyl5ebdTqLVzJ1WWDN7rUGdv2qDrHOlHeBMU32S6djwOizttFSwkj2q0FXAH8UFBTDS63gjSwJVzoSHaeFXo+6iT1b1eKj8sdxcbE3rA+\n-----END CERTIFICATE-----" \
#         ${ORG_URL?}


./internal/create-federation.sh "${IAM_TOKEN?}" "yc.yandex-team.federation" "yc.organization-manager.yandex" "yandex-cloud-federation" "Yandex.Cloud federation" "http://sts.yandex-team.ru/adfs/services/trust" "https://sts.yandex-team.ru/adfs/ls" ${ORG_URL?}
./internal/create-federation-cert.sh "${IAM_TOKEN?}" \
        "yc.yandex-team.federation" \
        "signing-2021" \
        "-----BEGIN CERTIFICATE-----\nMIIC4DCCAcigAwIBAgIQXUbsH179GqZIcObrUUUCrTANBgkqhkiG9w0BAQsFADAsMSowKAYDVQQDEyFBREZTIFNpZ25pbmcgLSBzdHMueWFuZGV4LXRlYW0ucnUwHhcNMjEwNTA3MTc0NTU0WhcNMjIwNTA3MTc0NTU0WjAsMSowKAYDVQQDEyFBREZTIFNpZ25pbmcgLSBzdHMueWFuZGV4LXRlYW0ucnUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC6ATWP5skePGwTcxKEJFoJTSQQyUbYJNyErNgDI+tXo4S8pvw4OPkyNtwIWheM0rpaxNAbNueQ8a6Q6T3qjVrdcgoOi/yEmXMOWcD2xPUXeZBMRXCxxyK86lYjh3Wsw9fHj5RnpBgdWZ1mGY/Smrr60HtpjD5dfLHCdImwVGS6YqHXqGrRTvHDthbgP4wEF0hSLFFKlk/7EdT4U4QC+iQq7umCLA0CJGFqh5cAz9Zdau9Jtox2RZOF1Jl6NT1vpRxnU1lUUq0D9mvYaqu2MqehKWWsfuiEdHa1ne7Id02bMt6E1WHqFcgCoYmp0WtgZyGUFTPSt95/tdcSB6RIYLdJAgMBAAEwDQYJKoZIhvcNAQELBQADggEBAKKnQP4p2UyWqEyGES0jpZZaJyyk4DdR32j5MxW6YRur/36wl9lI1urAORkcaUPKflvp15s+oxpBbftO9SpjafQqo3T4a7IZdLB5dAxNXJ0RTQvJP2vqAbUZbj8hUaD3aczUuvo59c3XvJKlVOufmayyjQhWUZpjRl2q6IJ1vZt+XZyuW+BLon5OO7uoj4Mgp4Jx3rmUhG2Y1JjuudiuuZwp0zHTcLfSD/xfv3g0xce5uG0L7qGMbGxsERplpOdMTRTIa/EgP1DYHe5qLhoHbqPGnrxl76GIf8tzwii7+2rMSqgYl0qmJ+fKv0n/Mo6+knsksB9qbVQlmotAvzPKxYU=\n-----END CERTIFICATE-----" \
         ${ORG_URL?}

grpcurl -insecure -H "Authorization: Bearer ${IAM_TOKEN?}" -d '{"federation_id":"yc.yandex-team.federation"}' ${ORG_URL?}:4290 yandex.cloud.priv.organizationmanager.v1.saml.CertificateService/List
