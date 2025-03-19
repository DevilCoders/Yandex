1) Заказываем сертификат на https://crt.yandex-team.ru/certificates?cr-form=1

CA: Certum (так сложилось исторически?)

Extended Validation, ECC сертификат - не нужно

TTL: 365 дней

Хосты (новые хосты прописывать сюда):

    api.ycp.cloud-preprod.yandex.net,
    *.api.ycp.cloud-preprod.yandex.net,
    api.cloud-preprod.yandex.net,
    *.api.cloud-preprod.yandex.net,
    api.canary.ycp.cloud-preprod.yandex.net,
    *.api.canary.ycp.cloud-preprod.yandex.net,
    console-preprod.cloud.yandex.ru,
    console-preprod.cloud.yandex.com,
    console.front-preprod.cloud.yandex.ru,
    console.front-preprod.cloud.yandex.com,

ABC: ycl7

2) Затем ждем письма что серт готов, и копируем сертификат

    $ ../../copy_cert.sh sec-newcert sec-01d45d0kgxhppgpjy54jk2hwmm

3) Катим

Проверяем что там те самые хосты

    $ openssl s_client -showcerts -connect [2a02:6b8:c0e:501:0:f806:0:a01]:443 </dev/null | openssl x509 -ext subjectAltName -nocert
