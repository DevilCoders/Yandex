from django import template
from core import settings as core_settings
from ui import settings as ui_settings


register = template.Library()


@register.assignment_tag
def core_setting(name):
    return getattr(core_settings, str(name))


@register.assignment_tag
def ui_setting(name):
    return getattr(ui_settings, str(name))
