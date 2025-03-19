#!/usr/bin/env bash
duty-bot.py -c /etc/yandex/duty-bot/config.yaml calendar --abc-sync $@
duty-bot.py -c /etc/yandex/duty-bot/config-strm.yaml calendar $@
duty-bot.py -c /etc/yandex/duty-bot/config-strm-event.yaml calendar $@
