При добавлении нового пользователя в облако, которое не принадлежит организации, ему автоматически назначается роль участника облака — `{{ roles-cloud-member }}`.

Роль актуальна только для облака, которое не принадлежит организации.

Эта роль необходима для доступа к ресурсам в облаке всем, кроме {% if audience != "internal" %}[владельцев облака](../resource-manager/concepts/resources-hierarchy.md#owner) и [сервисных аккаунтов](../iam/concepts/users/service-accounts.md){% else %}владельцев облака и сервисных аккаунтов{% endif %}.

Сама по себе эта роль не дает права выполнять какие-либо операции и используется только в сочетании с другими ролями, например, с `admin`, `editor` или `viewer`.