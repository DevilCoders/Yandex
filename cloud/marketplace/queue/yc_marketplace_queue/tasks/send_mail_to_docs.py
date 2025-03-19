from schematics.exceptions import BaseError
from typing import List
from typing import Union
from yc_common import config
from yc_common import logging

from cloud.marketplace.common.yc_marketplace_common.models.billing.publisher_account import PublisherAccountGetRequest
from cloud.marketplace.common.yc_marketplace_common.models.task import SendMailToDocsParams
from cloud.marketplace.common.yc_marketplace_common.models.task import Task
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.common.yc_marketplace_common.utils.billing import get_billing_client
from cloud.marketplace.queue.yc_marketplace_queue.lib.send_mail import internal_mail
from cloud.marketplace.queue.yc_marketplace_queue.utils.errors import SendMailBaseError

log = logging.get_logger("yc_marketplace_queue")

EMAIL_SUBJECT = "Новый договор ОБЛ ID клиента {}, договор № {}."
EMAIL_BODY = """
Коллеги привет!

Сформируйте, пожалуйста, договор для клиента.

С Уважением, YC Marketplace!
"""


def task(depends: List[Task], params: dict, *args, **kwargs) -> Union[TaskResolution, None]:
    publisher_id = None
    for t in depends:
        if "publisher_account_id" in t.response:
            publisher_id = t.response["publisher_account_id"]
            break

    if publisher_id is None:
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "Depends error",
                "message": "Cant found publisher_account_id in depends",
            })

    publisher = get_billing_client().get_publisher_account(publisher_id, PublisherAccountGetRequest(
        {"view": PublisherAccountGetRequest.ViewType.SHORT}))

    try:
        mail_params = SendMailToDocsParams(params["params"])
        mail_params.validate()
    except BaseError as e:
        log.exception("SendMailToDocsParams Validation failed")
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "TaskParamValidationError",
                "message": "Params validation failed with error: {}.".format(e),
            })

    try:
        destination = config.get_value("publisher_notification_email")
        internal_mail(EMAIL_SUBJECT.format(publisher.id, publisher.balance_contract_id), EMAIL_BODY, destination)
    except SendMailBaseError as e:
        log.exception("SendMailToDocs Cant send mail", e)
        return TaskResolution.resolve(
            status=TaskResolution.Status.FAILED,
            data={
                "code": "SendError",
                "message": "Send failed with error: {}.".format(e),
            })

    return TaskResolution.resolve(status=TaskResolution.Status.RESOLVED)
