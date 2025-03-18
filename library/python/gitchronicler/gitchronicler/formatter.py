# coding: utf-8

from datetime import datetime

from dateutil import tz


class ChangelogFormatter(object):

    ws_after_longest = 2

    def __init__(self, person_url='http://staff/%s'):
        self.person_url = person_url

    def version(self, version):
        return '%s\n%s\n' % (version, ('-' * len(version)))

    def _person(self, person):
        person_name, person_email = person
        if self.person_url and person_email:
            person_url = self.person_url % person_email
            return '[%s](%s)' % (person_name, person_url)
        else:
            return '%s' % person_name

    def author(self, author):
        author = self._person(author)
        return '\n* %s\n\n' % author

    def commit(self, change, max_row_len):
        subject = self._escape_markdown_syntax(change.message)

        if change.link:
            ws = ' ' * (max_row_len - len(change.message) + self.ws_after_longest)
            hub = '%s[ %s ]' % (ws, change.link)
        else:
            hub = ''

        return ' * %s%s\n' % (subject, hub)

    def maintainer(self, maintainer):
        maintainer = self._person(maintainer)
        dt = self.get_current_dt_with_tz_as_str()
        return '\n%s %s\n\n' % (maintainer, dt)

    def _escape_markdown_syntax(self, text):
        # http://daringfireball.net/projects/markdown/syntax#backslash
        # https://github.yandex-team.ru/tools/gitchronicler/issues/7

        lines = text.split('\n')
        for index, line in enumerate(lines):
            if line.startswith(('+', '-', '*', '#')):
                lines[index] = '\\' + line
        return '\n'.join(lines)

    def get_current_dt_with_tz_as_str(self):
        return datetime.now(tz.tzlocal()).replace(microsecond=0).isoformat(' ')
