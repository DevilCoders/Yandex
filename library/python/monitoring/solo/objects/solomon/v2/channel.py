# -*- coding: utf-8 -*-
import jsonobject

from library.python.monitoring.solo.objects.solomon.v2.base import SolomonObject


class Juggler(SolomonObject):
    host = jsonobject.StringProperty()
    service = jsonobject.StringProperty()
    description = jsonobject.StringProperty()
    instance = jsonobject.StringProperty(default="")
    tags = jsonobject.ListProperty(str)


class Telegram(SolomonObject):
    group_title = jsonobject.StringProperty(name="groupTitle", default="")
    login = jsonobject.StringProperty(name="login", default="")
    text_template = jsonobject.StringProperty(name="textTemplate", default="")
    send_screenshot = jsonobject.BooleanProperty(name="sendScreenshot", default=False)


def telegram_validator(telegram):
    if not telegram.login and not telegram.group_title:
        raise Exception("Method Telegram must have login or group_title property!")
    if telegram.login and telegram.group_title:
        raise Exception("Method Telegram can't provide both login and group_title properties!")


class SMS(SolomonObject):
    pass


class Email(SolomonObject):
    subject_template = jsonobject.StringProperty(name="subjectTemplate", required=False, default="")
    content_template = jsonobject.StringProperty(name="contentTemplate", required=False, default="")
    recipients = jsonobject.ListProperty(name="recipients", required=False, default=[])


def method_validator(method):
    if sum(method[_type] is not None for _type in ["juggler", "telegram", "sms", "email"]) != 1:
        raise Exception("Method provide more than one (or none) methods")


class Method(SolomonObject):
    juggler = jsonobject.ObjectProperty(Juggler, default=None, exclude_if_none=True)
    telegram = jsonobject.ObjectProperty(Telegram, default=None, exclude_if_none=True, validators=telegram_validator)
    sms = jsonobject.ObjectProperty(SMS, default=None, exclude_if_none=True)
    email = jsonobject.ObjectProperty(Email, default=None, exclude_if_none=True)


class Channel(SolomonObject):
    __OBJECT_TYPE__ = "Channel"

    _available_methods = ["juggler", "telegram", "sms"]
    id = jsonobject.StringProperty(name="id", required=True, default="")
    project_id = jsonobject.StringProperty(name="projectId", required=True, default="")
    name = jsonobject.StringProperty(name="name", required=True, default="0")
    version = jsonobject.IntegerProperty(name="version", required=True, default=0)
    notify_about_statuses = jsonobject.SetProperty(str, default={"NO_DATA", "ALARM", "ERROR", "OK"},
                                                   name="notifyAboutStatuses")
    repeat_notify_delay_millis = jsonobject.IntegerProperty(name="repeatNotifyDelayMillis", required=True, default=0)
    method = jsonobject.ObjectProperty(Method, required=True, validators=method_validator)
