"""
This file was generated with the custommenu management command, it contains
the classes for the admin menu, you can customize this class as you want.

To activate your custom menu add the following to your settings.py::
    ADMIN_TOOLS_MENU = 'mdbui.menu.CustomMenu'
"""

from django.urls import reverse
from django.conf import settings
from django.utils.translation import ugettext_lazy as _

from admin_tools.menu import Menu, items


class CustomMenu(Menu):
    """
    Custom Menu for mdbui admin site.
    """

    def __init__(self, **kwargs):
        Menu.__init__(self, **kwargs)
        self.children += [
            items.MenuItem(_('Dashboard'), reverse('admin:index')),
            items.MenuItem(
                'Installations',
                children=[
                    items.MenuItem('Porto Test', 'https://ui.db.yandex-team.ru'),
                    items.MenuItem('Porto Prod', 'https://ui-prod.db.yandex-team.ru'),
                    items.MenuItem('Compute Preprod', 'https://ui-preprod.db.yandex-team.ru'),
                    items.MenuItem('Compute Prod', 'https://ui-compute-prod.db.yandex-team.ru'),
                ],
            ),
        ]
        for key in settings.APP_MAP:
            if key in settings.INSTALLED_APPS:
                self.children += [
                    items.ModelList(_(key.split('.')[-1].capitalize()), models=(f'{key}.models.*',)),
                ]
