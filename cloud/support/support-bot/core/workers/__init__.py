#!/usr/bin/env python3
#
# Details about python-telegram-bot.JobQueue:
# https://github.com/python-telegram-bot/python-telegram-bot/wiki/Extensions-%E2%80%93-JobQueue
"""This module contains all workers for TelegramBot."""

import datetime
from core.constants import HOUR, MINUTE
from .notifier import InProgressNotifier, MaintenanceNotifier
from .watchdog import SecurityWatchdog
from .supkeeper import SupKeeper
from .task_notifier import TaskNotifier

# REPEATING WORKERS
# WorkerClass: (loop interval, run worker N seconds after bot start)
repeating_workers = {
#    SupportNotifier: (1 * MINUTE, 5),
    InProgressNotifier: (10 * MINUTE, 10),
    SecurityWatchdog: (12 * HOUR, 15),
    SupKeeper: (1 * MINUTE, 5),
    TaskNotifier: (1 * MINUTE, 5)

}

# DAILY WORKERS
# WorkerClass: datetime.time(hour, minutes, seconds) in UTC
daily_workers = {
    MaintenanceNotifier: datetime.time(9, 00, 00)  # UTC
}
