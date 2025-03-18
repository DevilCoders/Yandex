# coding: utf-8
import json
import os
import subprocess


class VcsError(Exception):
    def __init__(self, ctl, message=None):
        super(VcsError, self).__init__('{}: {}'.format(ctl.vcs.upper(), message))


class Change(object):
    def __init__(self, author_name, author_email, message, link):
        self.author_name = author_name
        self.author_email = author_email
        self.message = message
        self.link = link


class VcsCtl(object):
    vcs = None

    def get_output(self, command, args=None):
        """exec a vcs command and return the output"""
        args = args or []

        output = []

        popen = subprocess.Popen(
            [self.vcs, command] + args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        while popen.poll() is None:
            output += popen.stdout.readlines()
        ret = popen.poll()
        output += popen.stdout.readlines()
        output = [line.decode('utf-8') for line in output]
        return output, ret

    def get_changes(self, from_version):
        logs = self.get_logs(from_version)
        changes = [self.parse_log(log) for log in logs]
        return changes

    def is_initialized(self):
        _, ret = self.get_output('status')
        return not ret

    def get_logs(self, from_version):
        raise NotImplementedError

    def parse_log(self, log):
        raise NotImplementedError

    def get_maintainer(self):
        login = os.getenv('LOGNAME')
        return login, login


class GitCtl(VcsCtl):
    vcs = 'git'

    def get_logs(self, from_version):
        args = [
            '--no-merges',
            '--pretty=format:"%h;;;%an;;;%ae;;;%s"',
            '%s..HEAD' % (from_version or ''),
        ]
        lines, ret = self.get_output(command='log', args=args)
        if ret:
            raise VcsError(self, 'Error getting log')
        return (line.strip() for line in lines)

    def parse_log(self, log):
        if log.startswith('"'):
            # Строки, отдаваемые git log, могут быть обернуты в двойные кавычки.
            log = log[1:-1]
        commit_hash, author_name, author_email, subject = log.split(';;;', 3)
        return Change(
            author_name=author_name,
            author_email=author_email,
            message=subject.strip(),
            link=self.create_link(commit_hash),
        )

    def create_link(self, hash):
        remote_url = self.remote_url()
        if not remote_url:
            return

        if remote_url.endswith('.git'):
            remote_url = remote_url[:-len('.git')]
        if remote_url.startswith('git@'):
            host = remote_url[len('git@'):]
            host = host.replace(':', '/')
            remote_url = 'https://' + host
        return '{}/commit/{}'.format(remote_url, hash)

    def get_maintainer(self):
        return self.config_name(), self.config_email()

    def config_name(self):
        lines, ret = self.get_output(command='config', args=['user.name'])
        if ret:
            raise VcsError(self, 'Error getting config name')
        return lines[0].strip()

    def config_email(self):
        lines, ret = self.get_output(command='config', args=['user.email'])
        if ret:
            raise VcsError(self, 'Error getting config email')
        return lines[0].strip()

    def remote_url(self):
        """
        something like
        https://github.yandex-team.ru/tools/cab.git
        or
        git@github.yandex-team.ru:tools/cab.git
        """
        lines, ret = self.get_output(
            command='config',
            args=['--get', 'remote.origin.url'],
        )
        if ret:
            return
        return lines[0].strip()


class ArcCtl(VcsCtl):
    vcs = 'arc'

    def get_logs(self, from_version):
        output, ret = self.get_output(
            command='log',
            args=['./', '--json', '-n100'],
        )
        if ret:
            raise VcsError(self, 'Error getting log')
        logs = json.loads(''.join(output))

        for log in logs:
            if log['message'].find('releasing version {}'.format(from_version)) != -1:  # пока в arc нет тегов
                break
            yield log

    def parse_log(self, log):
        review_index = log['message'].find('\n\nREVIEW')
        if review_index != -1:
            log['message'] = log['message'][:review_index]

        return Change(
            author_name=log['author'],
            author_email=log['author'],
            message=log['message'],
            link=self.create_log_link(log),
        )

    def create_log_link(self, log):
        revision = log.get('revision')
        if revision is not None:
            return self.create_arc_revision_link(revision)
        return self.create_arc_vcs_commit_link(log['commit'])

    @staticmethod
    def create_arc_revision_link(revision):
        return 'https://a.yandex-team.ru/arc/commit/{}'.format(revision)

    @staticmethod
    def create_arc_vcs_commit_link(commit):
        return 'https://a.yandex-team.ru/arc_vcs/commit/{}'.format(commit)


def get_vcs_ctl():
    for ctl_class in (GitCtl, ArcCtl):
        ctl = ctl_class()
        if ctl.is_initialized():
            return ctl
