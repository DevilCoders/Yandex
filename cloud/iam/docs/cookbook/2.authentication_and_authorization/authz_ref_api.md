Для авторизации операции получения списка ссылок через [Reference API](https://wiki.yandex-team.ru/cloud/devel/reference-api/)
используется permission `<service>.<resource>.get` соответствующего ресурса.

Для авторизации операции установки новой ссылки используется permission `<service>.<resource>.updateReferences`, объектом
авторизации при этом является экземпляр ресурса типа `iam.resourceType` (передаваемый в поле `referrer.type`).

{% note info %}

Например, ресурс `Instance Group` ссылается на ресурс `Service Account`. В сервисе, владеющим ресурсом `Instance Group`,
при использовании ресурса `Service Account` проверяется permission `iam.serviceAccounts.use` на нем. Если проверка
завершилась успешно, сервис идет в Reference API ресурса `Service Account` от имени своего системного SA и устанавливает
ссылку с ресурса `Instance Group` (`referrer.type = "compute.instanceGroup"`) на ресурс `Service Account`. Reference API
ресурса `Service Account` проверяет permission `iam.serviceAccounts.updateReferences` на ресурсе `compute.instanceGroup`
типа `iam.resourceType`.

{% endnote %}

Для авторизации операции удаления ссылки используется permission `<service>.<resource>.updateReferences`, объектом
авторизации при этом является ресурс типа `iam.resourceType` (передаваемый в поле `referrer.type`).

{% note info %}

Например, ресурс `Compute Instance` ссылается на ресурс `Subnet`. При удалении связи сервис, владующий ресурсом
`Compute Instance`, идет в Reference API ресурса `Subnet` от имени своего системного SA и удаляет ссылку с ресурса
`Compute Instance` (`referrer.type = "compute.instance"`). Reference API ресурса `Subnet` проверяет permission
`vpc.subnets.updateReferences` на ресурсе `compute.instance` типа `iam.resourceType`.

{% endnote %}

{% note tip %}

Обсуждение правил авторизации Reference API можно найти в тикете [CLOUD-43138](https://st.yandex-team.ru/CLOUD-43138#5ed38ce87eebe86a0b8ffc26).

{% endnote %}
