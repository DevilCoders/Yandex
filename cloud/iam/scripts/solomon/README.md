Solomon Cloud projects generator
================================

May be used to prepare Solomon projects structure at Yandex.Cloud Solomon environments.

Usage
-----

Your need to get IamToken with admin rights for specified Solomon environment.
To get it, please follow [the instructions](https://cloud.yandex.ru/docs/iam/operations/iam-token/create).

`$IAM_TOKEN` variable is preferred, otherwise `yc` cli will try to generate it automatically.

To prepare project at specified solomon environment use:

```bash
# <ENVIRONMENT>=preprod|prod|testing|israel
./update.sh <ENVIRONMENT>
```

For example: `./update.sh preprod`

In order by manually delete any entity use `delete.sh`, e.g.
```
./delete.sh preprod alerts/openid-server-l7-unhealthy-preprod
```

Configs
-------

Solomon project configs located at [config.json](./config.json) file.

Read more
---------

* [Solomon API v2](https://wiki.yandex-team.ru/solomon/api/v2/)
* [Swagger Solomon API specification](https://solomon.yandex-team.ru/swagger-ui.html)
