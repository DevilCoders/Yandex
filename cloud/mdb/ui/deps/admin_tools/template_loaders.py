"""
Based on https://bitbucket.org/tzulberti/django-apptemplates/
and http://djangosnippets.org/snippets/1376/

Django template loader that allows you to load a template from a
specific application. This allows you to both extend and override
a template at the same time. The default Django loaders require you
to copy the entire template you want to override, even if you only
want to override one small block.

Template usage example::
    {% extends "admin/base.html" %}
"""

from os.path import join

from typing import Dict
from library.python.django.template.loaders.app_resource import Loader as ResourceLoader

_cache: Dict[str, str] = {}


class Loader(ResourceLoader):
    is_usable = True

    def get_template_sources(self, template_name, template_dirs=None):
        """
        Returns the absolute paths to "template_name" in the specified app.
        If the name does not contain an app name (no colon), an empty list
        is returned.
        The parent FilesystemLoader.load_template_source() will take care
        of the actual loading for us.
        """
        if ':' not in template_name:
            return []
        app_name, template_name = template_name.split(":", 1)
        # template_dirs = self.get_app_template_dirs(app_name)
        if not template_dirs:
            template_dirs = self.get_dirs()
        result = []

        for t_dir in template_dirs:
            try:
                from django.template import Origin

                origin = Origin(
                    name=join(t_dir, template_name),
                    template_name=template_name,
                    loader=self,
                )
            except (ImportError, TypeError):
                origin = join(t_dir, template_name)
            result.append(origin)
        return result
