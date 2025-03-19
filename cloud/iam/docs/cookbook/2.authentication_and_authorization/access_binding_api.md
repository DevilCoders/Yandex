Обычно ресурсы сервисов размещаются в контейнерах (например, фолдерах и облаках), права на ресурсы при этом наследуются
с этих контейнеров, т.е., выдав роль, например, на фолдер, она будет давать соответствующие права и на все ресурсы в
этом фолдере. Такая схема может быть неудобна пользователям, которые хотят большей гранулярности прав. Для этого в
сервисах могут быть поддержаны права непосредственно на ресурсы самого сервиса.

API по управлению такими правами должен быть реализован в самом сервисе [пример API прав на сервисные аккаунты](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/iam/v1/service_account_service.proto#53).

```proto
rpc ListAccessBindings (access.ListAccessBindingsRequest) returns (access.ListAccessBindingsResponse)

rpc SetAccessBindings (access.SetAccessBindingsRequest) returns (operation.Operation) {
  option (yandex.cloud.api.operation) = {
    metadata: "access.SetAccessBindingsMetadata"
    response: "google.protobuf.Empty"
  };
}

rpc UpdateAccessBindings (access.UpdateAccessBindingsRequest) returns (operation.Operation) {
  option (yandex.cloud.api.operation) = {
    metadata: "access.UpdateAccessBindingsMetadata"
    response: "google.protobuf.Empty"
  };
}
```

{% note info %}

Единое API по управлению правами всех ресурсов сделать нельзя, потому что наша ресурсная модель не предусматривает
единую точку регистрации ресурсов, соответственно, некоторые проверки (например, существование ресурса или наличие у
пользователя прав на управление правами на ресурсы) можно сделать только в самом сервисе, который отвечает за управление
ресурсами этого типа.

{% endnote %}

Необходимо завести два permission'а:
- `<service>.<resourceType>.listAccessBindings` — проверяется у вызывающего метод `ListAccessBindings` субъекта на ресурсе из запроса
- `<service>.<resourceType>.updateAccessBindings` — проверяется в методах `UpdateAccessBindings` и `SetAccessBindings`

Сервис самостоятельно проверяет наличие данных permission'ов у вызывающего субъекта. Перед проверкой прав необходимо проверить
существование ресурса с переданным в методы идентификатором и вернуть `NotFound` в случае его отсутствия.

Если все вышеуказанные проверки успешно пройдены, сервис должен спроксировать пользовательский запрос
[в соответствующие методы `AccessBindingService`](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/iam/v1/access_binding_service.proto#14)
(часть бэкенда IAM Control Plane).

{% note alert %}

Обратите внимание, что публичное API по управлению прав отличается от приватного. При вызове снаружи через API Gateway
не все параметры методов управления правами будут доступны пользователям.

{% endnote %}

Запрос в `AccessBindingService` делается от SA самого сервиса. Для этого ему необходима роль `internal.iam.accessBindings.admin`
на соответствующем ресурсе типа `iam.resourceType`.

{% note alert %}

Сейчас это можно сделать только через PR в iam-sync-configs. [Пример](https://bb.yandex-team.ru/projects/CLOUD/repos/iam-sync-configs/pull-requests/3208/diff#preprod.yaml).
Но в будущем выдача ролей на `iam.resourceType` должна уехать [в bootstrap-templates](https://st.yandex-team.ru/CLOUD-97809).

{% endnote %}
