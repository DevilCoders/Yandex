#!/bin/bash
set -e

ORGANIZATION_ID="fsfd379mujvg3ehrt0aa"
FEDERATION_ID="fsf530pvg7jc2psjj7rc"
ORG_URL=$1

if [[ -z ${ORG_URL} ]]; then
    echo "usage: $0 [ORG_URL]"
    ORG_URL=org.private-api.eu-central-1.aws.datacloud.net
    TS_URL=ts.private-api.eu-central-1.aws.datacloud.net
else
    TS_URL=${ORG_URL?}
fi

IAM_TOKEN=$(internal/get-eve-token.sh ${TS_URL})
echo $IAM_TOKEN

#echo Create organization
#internal/create-organization.sh "${IAM_TOKEN?}" "${ORGANIZATION_ID?}" "test-organization" "For tests" "${ORG_URL?}" | jq

: 'use YQL instead vvv
INSERT INTO `org/organizations`
(`id`,`created_at`,`description`,`display_name`,`name`,`settings`,`status`)
VALUES ('fsfd379mujvg3ehrt0aa',Timestamp('2021-01-01T00:00:00.000000Z'),'For tests', 'Test organization','test-organization','{}','ACTIVE');

INSERT INTO `hardware/default/identity/r3/clouds`
(`id`,`created_at`,`default_zone`,`description`,`name`,`organization_id`,`settings`,`status`,`user_listing_allowed`)
VALUES ('ecvqrb86sdvj72onh0uh',1609459200,'test-zone-a','Test','test-project','fsfd379mujvg3ehrt0aa','{"default_zone":"test-zone-a"}','ACTIVE',True);
INSERT INTO `hardware/default/identity/r3/cloud_names_non_unique`
(`id`,`created_at`,`default_zone`,`description`,`name`,`organization_id`,`settings`,`status`,`user_listing_allowed`)
VALUES ('ecvqrb86sdvj72onh0uh',1609459200,'test-zone-a','Test','test-project','fsfd379mujvg3ehrt0aa','{"default_zone":"test-zone-a"}','ACTIVE',True);
INSERT INTO `hardware/default/identity/r3/clouds_organization_index`
(`id`,`created_at`,`default_zone`,`description`,`name`,`organization_id`,`settings`,`status`,`user_listing_allowed`)
VALUES ('ecvqrb86sdvj72onh0uh',1609459200,'test-zone-a','Test','test-project','fsfd379mujvg3ehrt0aa','{"default_zone":"test-zone-a"}','ACTIVE',True);
UPSERT INTO `resource_manager/cloud_registry`
    ( `id`, `created_at`, `organization_id` )
VALUES ('ecvqrb86sdvj72onh0uh',Timestamp('2021-01-01T00:00:00.000000Z'),'fsfd379mujvg3ehrt0aa' );


INSERT INTO `hardware/default/identity/r3/folders`
(`id`,`cloud_id`,`description`,`name`,`labels`,`settings`,`status`,`created_at`,`modified_at`)
VALUES ('ecvqrb86sdvj72onh0uh','ecvqrb86sdvj72onh0uh','Test','test-project','{}','{}','ACTIVE',1609459200,1609459200);
INSERT INTO `hardware/default/identity/r3/folder_names`
(`id`,`cloud_id`,`description`,`name`,`labels`,`settings`,`status`,`created_at`,`modified_at`)
VALUES ('ecvqrb86sdvj72onh0uh','ecvqrb86sdvj72onh0uh','Test','test-project','{}','{}','ACTIVE',1609459200,1609459200);
INSERT INTO `hardware/default/identity/r3/folders_cloud_index`
(`id`,`cloud_id`,`description`,`name`,`labels`,`settings`,`status`,`created_at`,`modified_at`)
VALUES ('ecvqrb86sdvj72onh0uh','ecvqrb86sdvj72onh0uh','Test','test-project','{}','{}','ACTIVE',1609459200,1609459200);
UPSERT INTO `resource_manager/folder_registry`
    ( `id`, `cloud_id`, `created_at`)
VALUES ('ecvqrb86sdvj72onh0uh','ecvqrb86sdvj72onh0uh',Timestamp('2021-01-01T00:00:00.000000Z'));
'use YQL instead ^^^


./internal/create-federation.sh "${IAM_TOKEN?}" ${FEDERATION_ID?} ${ORGANIZATION_ID?} "test-federation" "Test federation" "https://keycloak-test.cloud.yandex.net:8443/auth/realms/master" "https://keycloak-test.cloud.yandex.net:8443/auth/realms/master/protocol/saml" ${ORG_URL?}
./internal/create-federation-cert.sh "${IAM_TOKEN?}" \
        ${FEDERATION_ID?} \
        "cert" \
         "-----BEGIN CERTIFICATE-----\nMIICmzCCAYMCBgF10IaBfjANBgkqhkiG9w0BAQsFADARMQ8wDQYDVQQDDAZtYXN0ZXIwHhcNMjAxMTE2MTAwNjE3WhcNMzAxMTE2MTAwNzU3WjARMQ8wDQYDVQQDDAZtYXN0ZXIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCqmx9Zm108vtI2jzDd9yLECF6SHAGDR4231e5wbcoYP+4z5//jGGakG4j+NoZH1C6D6QJPQchILrtJAAM5cabxI1dzNiv+kk9RXMlMxmT3Q5jq0PiCpEPTIqXIRT9/RPBz2zP4ITYLV7IFNT8d6tYD8RwFYKmiAdm0bAqQ4dqkKZ3wQoH1/YCBHzMGRHRgDset7a62YBHHhzOfIjct5L3mmO1FEt2utSclxcDwY3e8oTzieCH5OYoD3OvBhvXsfsAZ9mB90fyoCF60HKefM2oEdTGy+avI/665YaP4gmh+gTUvjkfaBd7ntYK0TwJmikZ4A/u9h05VbcaBldPNQ0E5AgMBAAEwDQYJKoZIhvcNAQELBQADggEBAErlLaDWhu8Ce/JojCsE0if0rXZiCt7d7wF9wnpI8HF8viJ8pKyXyEuasX3ZahL4l3CtsDub6GTF9+SECOEhJZpMmQrylVMYmwACLxbNhiIN8a/wiD1SpoSy/wJCZE0Ddh88DHYWTwGAZrC2Qng5ibbymWUmrhrt4ovU7/Tdg/UEGDMYZEj6KogN0eGfUdw1NStU5QjGqOHcbLHNFtvSsc7+Z8eXXz1+zkANr0wAi7O6fUHbpEaPOUnn6WlbK9guv2ro7PcpoRufR0v/HsqDB3KJEK3K+QEkbcsQwFJqavZy8TOELU+Th58BIuBhisDCri/95Xj1FY41/pV9zwdcaus=\n-----END CERTIFICATE-----" \
         ${ORG_URL?}
# old cert "-----BEGIN CERTIFICATE-----\nMIIDjzCCAnegAwIBAgIUF3bSIPKEcz0+93czW8h814WWGl8wDQYJKoZIhvcNAQELBQAwVzELMAkGA1UEBhMCUlUxDzANBgNVBAgMBk1vc2NvdzEPMA0GA1UECgwGWWFuZGV4MRUwEwYDVQQLDAxZYW5kZXguQ2xvdWQxDzANBgNVBAMMBllhbmRleDAeFw0yMDAyMTIwODI2NTVaFw0yNTAyMTIwODI2NTVaMFcxCzAJBgNVBAYTAlJVMQ8wDQYDVQQIDAZNb3Njb3cxDzANBgNVBAoMBllhbmRleDEVMBMGA1UECwwMWWFuZGV4LkNsb3VkMQ8wDQYDVQQDDAZZYW5kZXgwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDYVcxqC0Ienc85C2oD93GvYB172VSPYuy7umjHa/8Xy8KUGmAjO1pJA2Qvgrs0AmymueM6d6Uw0C7MsDY/Z3eOid9ZRnbmJtfx2MzmyA91Y0ZvcIxYXWa4kloEiLUMGs94ixgBat+erKN836NVF4mFOtLUsueMOkQkdFw6RPlw9NccEmWxb/XfzA2fceCjWFeJt1RQSAMPxmsd+s9NmsaZ7dFsn1Vx/ACLdlzxhyCjf7A5FIRZUxEHQF5UOP7z2bKkErDivFj19XhZ2whHmHNKy2pvrzZ0ufoy2+isW5HzEn1+DO3hwX8pKOOvjtKi5vqtsdjteGDmF2+lm+RxrkL/AgMBAAGjUzBRMB0GA1UdDgQWBBRVWZqkoCfZHnN6vDN6d5l2tQGp7jAfBgNVHSMEGDAWgBRVWZqkoCfZHnN6vDN6d5l2tQGp7jAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQA4/gJ1/dawiBYa4hYpCFsnWIgoJ9iLgG6mOZ6q0xT8X/zAPkA9EeB75OdFPFruAydRlihdiVfrjbdOYoEgrTbF1jtWAAK+YQpLohrLp070h5PzXMMvso8Tkg4phxLEGhtsHg+SMFipvg0uTG1CWttgnHy8tWfrjbrF2MPA846ibHu7nPjhwoQxdd5kILPYLTANc3YzwaiN96f6flZgedkVDUHMT2AVMpFvbYb1jNqULQ9Xyl5ebdTqLVzJ1WWDN7rUGdv2qDrHOlHeBMU32S6djwOizttFSwkj2q0FXAH8UFBTDS63gjSwJVzoSHaeFXo+6iT1b1eKj8sdxcbE3rA+\n-----END CERTIFICATE-----" \
