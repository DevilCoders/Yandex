#!/usr/bin/env python
# coding: utf-8
from django.core import management


if __name__ == "__main__":
    management._commands = dict.fromkeys(['tankerupload', 'tankerdownload'],
                                          'django_tanker')

    management.execute_from_command_line()
