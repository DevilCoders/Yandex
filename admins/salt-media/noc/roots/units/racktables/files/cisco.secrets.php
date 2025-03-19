<?php ob_start(); ?>
# Syntax:
# <endpoint|*> <telnet> <hostname|-> <port|-> <username|-> <line password> <enable password>
# FIXME: <endpoint|*> <rsh> <username>
# S-T-A-R-T
s*.yndx.net telnet - - racktables {{ pillar['sec_rt-yandex']['yandex_terminal_password'].split(' ')[0] }} -
ben-*.yndx.net telnet - - racktables {{ pillar['sec_rt-yandex']['yandex_terminal_password'].split(' ')[0] }} -
ekb-*.yndx.net telnet - - racktables {{ pillar['sec_rt-yandex']['yandex_terminal_password'].split(' ')[0] }} -
red[123]-*.yndx.net telnet - - racktables {{ pillar['sec_rt-yandex']['yandex_terminal_password'].split(' ')[0] }} -
*-u[0-9]*.yndx.net telnet - - racktables {{ pillar['sec_rt-yandex']['yandex_terminal_password'].split(' ')[0] }} -
*-*u[0-9]*.yndx.net telnet - - racktables {{ pillar['sec_rt-yandex']['yandex_terminal_password'].split(' ')[0] }} -
*-*s[0-9]*.yndx.net telnet - - racktables {{ pillar['sec_rt-yandex']['yandex_terminal_password'].split(' ')[0] }} -
*-[0-9]*a[0-9]*.yndx.net telnet - - racktables {{ pillar['sec_rt-yandex']['yandex_terminal_password'].split(' ')[0] }} -
*-[0-9]*s[0-9]*.yndx.net telnet - - racktables {{ pillar['sec_rt-yandex']['yandex_terminal_password'].split(' ')[0] }} -
*-*a[0-9]*.yndx.net telnet - - racktables {{ pillar['sec_rt-yandex']['yandex_terminal_password'].split(' ')[0] }} -
# S-T-O-P
<?php ob_end_clean(); ?>
