# Отмена удаления облака

В этом разделе приведена инструкция по отмене удаления облака в случае, если ранее вы ошибочно [инициировали удаление](delete.md). {% if product == "yandex-cloud" %}Об отмене удаления облака при удалении из-за задолженности читайте в разделах [Цикл оплаты для физических лиц](../../../billing/payment/billing-cycle-individual.md) и [Цикл оплаты для организаций и ИП](../../../billing/payment/billing-cycle-business.md).{% endif %}

Отменить удаление можно, пока облако находится в статусе ожидания удаления `PENDING_DELETION`.

{% if product == "yandex-cloud" %}

{% include [alert-pending-deletion](../../../_includes/resource-manager/alert-pending-deletion.md) %}

{% endif %}

Чтобы отменить удаление облака, у вас должна быть роль `{{ roles-cloud-owner }}` на это облако. Если вы не можете выполнить эту операцию, обратитесь к [владельцу облака](../../concepts/resources-hierarchy.md#owner).

{% list tabs %}

- Консоль управления

  1. Откройте [консоль управления]({{ link-console-main }}).
  
  1. Выберите облако в списке и нажмите на значок ![***](../../../_assets/options.svg) в правом верхнем углу страницы. В отобразившемся меню выберите **Отменить удаление**.

{% endlist %}

Начатая ранее операция удаления облака будет отменена.
