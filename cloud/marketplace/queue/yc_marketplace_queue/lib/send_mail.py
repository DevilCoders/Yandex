import os
import smtplib
from email.message import EmailMessage

from yc_common import config
from yc_common import logging
from cloud.marketplace.queue.yc_marketplace_queue.utils.errors import SendMailSendError

log = logging.get_logger(__name__)


RELAY = "outbound-relay.yandex.net"

# LOGIN = os.getenv("MARKETPLACE_SENDMAIL_LOGIN")
# PASS = os.getenv("MARKETPLACE_SENDMAIL_PASS")


def internal_mail(subject, body, destination):
    server = smtplib.SMTP(RELAY)
    internal_reply_to = config.get_value("marketplace.reply")

    # try:
    #     server.login(LOGIN, PASS)
    # except smtplib.SMTPAuthenticationError as e:
    #     server.quit()
    #     raise SendMailAuthenticationError("Authentication failed with error: {}.".format(e))

    msg = EmailMessage()
    msg.set_content(body)

    msg["Subject"] = subject
    msg["Reply-To"] = internal_reply_to
    msg["From"] = "{}@yandex-team.ru".format(os.getenv("MARKETPLACE_SENDMAIL_LOGIN"))
    msg["To"] = destination

    resp = server.send_message(msg)
    server.quit()
    if destination in resp:
        raise SendMailSendError("Send failed with error: {}.".format(resp))
