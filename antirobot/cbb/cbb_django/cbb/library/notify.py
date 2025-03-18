import collections
import datetime
import logging
import socket
import traceback
from functools import wraps
import hashlib

from antirobot.cbb.cbb_django.cbb.models import GroupResponsible
from django.conf import settings
from django.core.mail import send_mail
from django.template import loader


MAIL_SUFFIX = "@yandex-team.ru"


class ChangesRegistry:
    def __init__(self, request):
        self.author = request.cbb_user.login
        self.changes = collections.defaultdict(list)

    def clear_changes(self):
        self.changes = collections.defaultdict(list)

    def __log(self, text, group_id, info=None):
        self.changes[group_id].append({
            "timestamp": datetime.datetime.now(),
            "text": text,
            "info": info or {},
        })

    def log_group_added(self, group_id, name, expire, description):
        self.__log(
            "group has been created",
            group_id,
            {"group_name": name, "group_expire": expire, "group_description": description}
        )

    def log_group_removed(self, group_id):
        self.__log(
            "group has been removed",
            group_id,
        )

    def log_metadata_change(self, group_id, name, expire, description):
        self.__log(
            "group metadata has been updated",
            group_id,
            {"group_name": name, "group_expire": expire, "group_description": description}
        )

    def log_responsible_added(self, group_id, login):
        self.__log("new responsible has been added", group_id, {"login": login})

    def log_responsible_removed(self, group_id, login):
        self.__log("responsible has been removed", group_id, {"login": login})

    def log_ip_range_added(self, group_id, rng_start, rng_end, expire, description):
        self.__log(
            "IP range has been added",
            group_id,
            {
                "range_start": rng_start,
                "range_end": rng_end,
                "range_expire": expire,
                "range_description": description,
            }
        )

    def log_ip_range_removed(self, group_id, rng_start, rng_end, description):
        self.__log(
            "IP range has been removed",
            group_id,
            {
                "range_start": rng_start,
                "range_end": rng_end,
                "range_description": description,
            }
        )

    def log_txt_range_added(self, group_id, rng_txt, expire, description):
        self.__log(
            "regex has been added",
            group_id,
            {
                "range_txt": rng_txt,
                "range_expire": expire,
                "range_description": description,
            }
        )

    def log_txt_range_removed(self, group_id, rng_txt_id, description):
        self.__log(
            "regex has been removed",
            group_id,
            {
                "range_txt_id": rng_txt_id,
                "range_description": description,
            }
        )

    def log_txt_range_moved(self, group_id_from, group_id_to, rng_txt_id, description, old_bin, new_bin):
        self.__log(
            "regex has been moved",
            group_id_to,
            {
                "range_txt_id": rng_txt_id,
                "range_description": description,
                "from_group": group_id_from,
                "old_bin": str(old_bin),
                "new_bin":  str(new_bin),
            }
        )

    def log_txt_range_copied(self, group_id_from, group_id_to, rng_txt_id, description, old_bin, new_bin):
        self.__log(
            "regex has been copied",
            group_id_to,
            {
                "range_txt_id": rng_txt_id,
                "range_description": description,
                "from_group": group_id_from,
                "old_bin": str(old_bin),
                "new_bin":  str(new_bin),
            }
        )

    def log_network_added(self, group_id, net_ip, net_mask, description):
        self.__log(
            "network has been added",
            group_id,
            {
                "network_ip": net_ip,
                "network_mask": net_mask,
                "network_description": description,
            }
        )

    def log_network_removed(self, group_id, net_ip, net_mask, description):
        self.__log(
            "network has been removed",
            group_id,
            {
                "network_ip": net_ip,
                "network_mask": net_mask,
                "network_description": description,
            }
        )

    def send_notifications(self):
        changes_hash = hashlib.sha256()
        for group_id in sorted(self.changes):
            changes_hash.update(str(group_id).encode("utf-8"))
            for change in self.changes[group_id]:
                values_str = "".join(map(str, change["info"].values()))
                changes_hash.update((change["text"] + values_str).encode("utf-8"))

        for group_id in sorted(self.changes):
            mail_to = [row.user_login + MAIL_SUFFIX for row in GroupResponsible.query.filter_by(group_id=group_id)]
            title = f"{settings.MAIN_CBB_HOST}: change in group {group_id} by {self.author}"
            if settings.MAIN_CBB_HOST == "cbb-ext.yandex-team.ru":
                mail_to.append("cbb" + MAIL_SUFFIX)  # mail list for cbb
            group_changes = sorted(self.changes.pop(group_id), key=lambda c: c["timestamp"])

            html_message = loader.render_to_string(
                "cbb/mail.html",
                {
                    "title": title,
                    "cbb_host": settings.MAIN_CBB_HOST,
                    "author": self.author,
                    "group_id": group_id,
                    "changes": group_changes,
                    "hostname": socket.gethostname(),
                    "changes_hash": changes_hash,
                },
            )

            try:
                send_mail(
                    title,
                    None,
                    settings.FROM_EMAIL,
                    mail_to,
                    html_message=html_message,
                )
                logging.info(f"Mail {html_message} was sent to {mail_to}")
            except:
                logging.error(f"Error while sending message to {mail_to} {traceback.format_exc()}")


def notify_responsibles(foo):
    @wraps(foo)
    def wrapper(request, *args, **kwargs):
        request.cbb_changes = ChangesRegistry(request)
        result = foo(request, *args, **kwargs)

        if not request.method == "POST":
            return result

        request.cbb_changes.send_notifications()

        return result
    return wrapper
