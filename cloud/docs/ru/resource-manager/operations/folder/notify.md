# Уведомления об инцидентах в каталоге

Уведомления об инцидентах по определенным каталогам в облаке получают пользователи, указанные в настройках каталога в разделе **Уведомления об инцидентах**. Владельцы каталогов по умолчанию уведомлений не получают, им рекомендуется самостоятельно на них подписаться.

Чтобы добавить пользователя в список адресатов уведомлений:

{% list tabs %}

- Консоль управления

  1. В [консоли управления]({{ link-console-cloud }}) выберите каталог для настройки. Если необходимо, [переключитесь на другое облако](../cloud/switch-cloud.md).
  1. Перейдите на вкладку **Уведомления об инцидентах**.
  1. Нажмите кнопку **Добавить**.
  1. Выберите пользователя, которого хотите оповещать об инцидентах, и нажмите кнопку **Добавить**.

     {% note info %}

     Добавлять можно пользователей с [аккаунтом {% if product == "yandex-cloud" %}на Яндексе{% endif %}{% if product == "cloud-il" %}Google{% endif %}](../../../iam/concepts/index.md#passport) и [федеративных пользователей](../../../iam/concepts/index.md#saml-federation). Федеративные пользователи должны в собственных настройках учетной записи указать адрес электронной почты.
     
     {% endnote %}

{% endlist %}
