#!/usr/bin/env python3

import re
import sys


def main():
    if len(sys.argv) < 2:
        print("Usage: calendar_extractor.py <saved_calendar_page.html>")
        print("Run with -h/--help to print fine user manual")
        sys.exit(1)

    if sys.argv[1] == "-h" or sys.argv[1] == "--help":
        print(
            """How to use:
Firefox:
    - Open calendar page
    - Right-click and `Inspect`,
    - Find html or 'body' element, right-click and select `Copy -> Copy inner HTML`
    - Save to any file, e.g. calendar.html
    - Run script:
        python3 calendar_extractor.py calendar.html
    - Insert dumped emails list into your mail client/web mail
"""
        )
        sys.exit(0)

    contents = ""
    with open(sys.argv[1]) as f:
        contents = f.read()

    logins = re.findall(r"staff.yandex-team.ru/([A-Za-z0-9-]+)", contents)
    if not logins:
        print("Sorry, no staff logins found")

    logins = sorted(list(set(logins)))

    for login in logins:
        if login in ["map"]:
            continue

        print("{}@yandex-team.ru".format(login))


if __name__ == "__main__":
    main()
