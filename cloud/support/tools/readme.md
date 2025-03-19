# Yandex.Cloud Support Tools

### Manual setup

* Clone repos: `svn co svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/cloud/support/tools`
* Run `ya make` for needed tool
* [Get oAuth-token](https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb)
* Generate config and download cert: `saint --init` or `quotactl --init`

## saint

**Yandex.Cloud Support All in oNe Tool**

Cloud resolve and resources info.

For help use `saint --help`

[Documentation](https://wiki.yandex-team.ru/cloud/support/man/tools/#saintsaint)

## quotactl

**Yandex.Cloud Quota Manager for support team ([YCLOUD-1460](https://st.yandex-team.ru/YCLOUD-1460))**

Interactive mode: `quotactl`

CLI mode: `quotactl --help`

PRE-PROD env: `quotactl --preprod`

[Documentation](https://wiki.yandex-team.ru/cloud/support/man/tools/#quota-managerquotactl)

## Notify
