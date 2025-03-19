#!/usr/bin/env python3

from telegram.ext import CommandHandler, CallbackQueryHandler
from .commands import (start, bye, settings, menu, in_progress, cloudops_force_notify,
                       cloudsupport_force_notify, help_message, schedule, debug_message, duty, testapi,quality)

from .buttons import main_button


command_handlers = [
    CommandHandler('start', start),
    CommandHandler('menu', menu),
    CommandHandler(['bye', 'delete'], bye),
    CommandHandler('settings', settings),
    CommandHandler(['in_progress', 'me'], in_progress),
    CommandHandler(['open', 'force_notify'], cloudsupport_force_notify),
    CommandHandler('ops', cloudops_force_notify),
    CommandHandler('help', help_message),
    CommandHandler(['schedule', 'team'], schedule),
    CommandHandler(['debug', 'log', 'logs'], debug_message),
    CommandHandler(['duty'], duty),
    CommandHandler(['test'], testapi),
    CommandHandler(['quality'], quality),
]

button_handlers = [
    CallbackQueryHandler(main_button),
]
    