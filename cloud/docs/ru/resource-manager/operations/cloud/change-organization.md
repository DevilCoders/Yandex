# Изменение организации для облака

{% if product == "yandex-cloud" %}
{% note info %}

В [консоли управления]({{ link-console-main }}) доступен выбор интерфейса навигации между ресурсами и сервисами. Чтобы сменить способ навигации, в блоке с информацией аккаунта нажмите кнопку ![image](../../../_assets/settings.svg), затем выберите ![image](../../../_assets/experiments.svg) **Эксперименты** и включите опцию **Новая навигация**.

{% endnote %}

Чтобы изменить назначенную облаку организацию:

{% list tabs %}

- Старая навигация

  1. В [консоли управления]({{ link-console-main }}) нажмите кнопку ![***](../../../_assets/options.svg) напротив нужного облака и выберите **Изменить организацию**.

      ![image](../../../_assets/iam/cloud-actions.png)

  1. Выберите новую организацию из списка и нажмите **Изменить**.

- Новая навигация

{% endif %}
{% if product == "cloud-il" %}
{% note info %}

   На стадии [Preview](../../../overview/concepts/launch-stages.md) действует ограничение: доступна только 1 организация и 1 облако.

{% endnote %}

{% list tabs %}
- Консоль управления

{% endif %}
  1. В [консоли управления]({{ link-console-main }}) выберите облако в списке слева.
  1. В правом верхнем углу нажмите кнопку ![***](../../../_assets/options.svg) и выберите **Изменить организацию**.

      ![image](../../../_assets/iam/change-organization-n-n.png)

  1. Выберите новую организацию из списка и нажмите **Изменить**.

{% endlist %}