# coding: utf-8

import io
import os
import shutil
import subprocess

from gitchronicler import formatter
from gitchronicler.ctl import get_vcs_ctl


class ChroniclerNextVersionException(Exception):
    pass


class Chronicler(object):
    def __init__(
        self,
        changelog_path='changelog.md',
        editor=None,
        formatter_options=None,
    ):
        self._repository_ctl = None
        self.changelog_path = changelog_path
        self.editor = editor or os.getenv('VISUAL', os.getenv('EDITOR', 'vim'))
        formatter_options = formatter_options or {}
        self.changelog_formatter = formatter.ChangelogFormatter(**formatter_options)

        self.cl_exists = os.path.exists(self.changelog_path)

    @property
    def repository_ctl(self):
        if not self._repository_ctl:
            self._repository_ctl = get_vcs_ctl()
        return self._repository_ctl

    def get_current_version(self):
        if self.cl_exists:
            with open(self.changelog_path, 'r') as cr:
                return cr.readline().strip()
        else:
            return None

    def get_changelog_records(self):
        # Возвращает пары (версия, запись в changelog)
        if not self.cl_exists:
            return

        version = None
        prev_line = None
        record_lines = []

        def lines_to_record(lines):
            return ''.join(lines).strip('\n')

        for line in io.open(self.changelog_path, 'r', encoding='utf-8'):
            if line.startswith('-'):
                if version:
                    yield version, lines_to_record(record_lines[:-1])
                record_lines = []
                version = prev_line.strip()
            else:
                prev_line = line
                record_lines.append(line)
        yield version, lines_to_record(record_lines)

    def get_latest_changelog_record(self):
        return next(
            self.get_changelog_records(),
            (None, None)
        )

    def get_grouped_changes(self, from_version):
        changes = self.repository_ctl.get_changes(from_version)

        grouped_changes = {}
        for change in changes:
            grouped_changes.setdefault(
                (change.author_name, change.author_email), []
            ).append(change)

        return grouped_changes

    def get_changelog_record(
        self,
        from_version=None,
        to_version=None,
        release_type=None,
        versioning_schema=None,
    ):
        from_version = from_version or self.get_current_version()
        to_version = to_version or self.get_next_version(
            version=from_version,
            release_type=release_type,
            versioning_schema=versioning_schema,
        )

        cf = self.changelog_formatter
        grouped_changes = self.get_grouped_changes(from_version)
        maintainer = self.repository_ctl.get_maintainer()

        lines = []

        # часто просиходит, что в релизе коммиты только от собирающего релиз
        # кажется, что в этом случае будет лучше не писать одного и того же
        # человека два раза
        if len(grouped_changes) == 1 and list(grouped_changes.keys())[0] == maintainer:
            should_add_author_line = False
        else:
            should_add_author_line = True

        lines.append(cf.version(to_version))
        for author, changes in grouped_changes.items():
            if should_add_author_line:
                lines.append(cf.author(author))

            max_row_len = max(len(change.message) for change in changes)
            for change in changes:
                commit = cf.commit(change, max_row_len)
                lines.append(commit)

        lines.append(cf.maintainer(maintainer))
        return ''.join(lines)

    def write_changelog(
        self,
        from_version=None,
        to_version=None,
    ):
        from_version = from_version or self.get_current_version()
        to_version = to_version or self.get_next_version(from_version)

        tmp_changelog = self.changelog_path + '_tmp.md'
        with open(tmp_changelog, 'w') as cw:
            record = self.get_changelog_record(from_version, to_version)
            cw.write(record.encode('utf-8'))

            if self.cl_exists:
                with open(self.changelog_path, 'r') as cr:
                    shutil.copyfileobj(cr, cw)

        subprocess.call([self.editor, tmp_changelog])
        if self.cl_exists:
            os.unlink(self.changelog_path)
        os.rename(tmp_changelog, self.changelog_path)
        self.cl_exists = True

    def get_next_version(
        self,
        version=None,
        release_type='minor',
        versioning_schema=None,
    ):
        version = version or self.get_current_version() or '0.0'

        # Guess delimiter
        delimiter = ''
        if '-' in version:
            delimiter = '-'
        elif '.' in version:
            delimiter = '.'

        if versioning_schema:
            # Decide witch part to increase
            version_parts = version.split(delimiter)
            version_schema = versioning_schema.split(delimiter)

            if len(version_parts) != len(version_schema):
                raise ChroniclerNextVersionException(
                    "Version `{}` doesn't match schema `{}`".format(version, versioning_schema)
                )

            if release_type and release_type not in version_schema:
                raise ChroniclerNextVersionException(
                    "Unknown release type `{}` in versioning schema `{}`".format(release_type, versioning_schema)
                )

            if release_type in version_schema:
                increase_version_part = version_schema.index(release_type)
            else:  # If no release_type supplied - increasing right number
                increase_version_part = len(version_parts) - 1

            version_parts[increase_version_part] = str(int(version_parts[increase_version_part]) + 1)
            for reset_version_part in range(increase_version_part + 1, len(version_schema)):
                version_parts[reset_version_part] = '0'

            version = delimiter.join(version_parts)
        else:
            if delimiter:
                major, minor = version.rsplit(delimiter, 1)
            else:
                major, minor = '', version
            version = '%s%s%d' % (major, delimiter, int(minor) + 1)

        return version


chronicler = Chronicler()
