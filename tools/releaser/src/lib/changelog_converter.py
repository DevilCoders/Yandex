# coding: utf-8

import codecs
import re
from email.utils import parseaddr

from dateutil import parser
from click import echo

from tools.releaser.src.lib.deblibs.changelog import Changelog


def convert_changelog(debian_changelog, markdown_changelog, trace=False):
    changelog = open(debian_changelog, 'r')
    changelog_entries = Changelog(changelog)
    if trace:
        echo("Converting old changelog...")
    new_changelog = codecs.open(markdown_changelog, 'w', 'utf8')

    for entry in changelog_entries:
        timestamp = parser.parse(entry.date).strftime("%Y-%m-%d %H:%M:%S")
        author_name = parseaddr(entry.author)[0]
        author_email = parseaddr(entry.author)[1]
        new_changelog.write(str(entry.version) + "\n---\n")
        for message in entry.changes():
            if trace:
                echo(message)
            if message.startswith("  ["):
                person_name = re.search(r"\[(.+?)\]", message).group(1)[1:-1]
                person_name_encoded = person_name.replace(" ", "%20")
                new_changelog.write(
                    " * [" + person_name + "](https://staff.yandex-team.ru/" + person_name_encoded + ")\n\n")
            else:
                new_changelog.write(message.replace("_", r"\_") + "\n")
        new_changelog.write(
            " [" + author_name + "](https://staff.yandex-team.ru/" + author_email + ") " + timestamp + "\n\n")
    new_changelog.close()
    if trace:
        echo("Conversion of changelog completed.")
