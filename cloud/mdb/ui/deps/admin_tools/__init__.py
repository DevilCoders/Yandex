"""
django-admin-tools is a collection of extensions/tools for the default django
administration interface, it includes:

 * a full featured and customizable dashboard,
 * a customizable menu bar,
 * tools to make admin theming easier.
"""

VERSION = '0.9.1'

try:
    import django

    if django.VERSION < (3, 2):
        default_app_config = 'admin_tools.apps.AdminToolsConfig'
except ImportError:
    pass
