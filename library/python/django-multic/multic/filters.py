# coding: utf-8
from __future__ import unicode_literals

import django_filters
from django.db import models


def build_filter_cls(model, filter_fields=None):
    """
    Используем https://django-filter.readthedocs.org/
    """
    filter_cls = type(
        str(model._meta.object_name + 'Filter'),
        (django_filters.FilterSet,),
        {
            'Meta': type(
                str('Meta'),
                (),
                {
                    'model': model,
                    'fields': filter_fields,
                    'filter_overrides': {
                        models.CharField: {
                            'filter_class': django_filters.CharFilter,
                            'extra': lambda f: {
                                'lookup_expr': 'iexact',
                            },
                        }
                    }
                }
            )
        }
    )

    return filter_cls
