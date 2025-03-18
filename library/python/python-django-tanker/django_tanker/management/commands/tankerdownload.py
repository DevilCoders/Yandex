# coding: utf-8
from django.utils.encoding import force_text, smart_str

from django_tanker.management.commands import DownloadUploadBaseCommand


class Command(DownloadUploadBaseCommand):
    help = u'Downloads *.po files from the Tanker instance'

    def add_arguments(self, parser):
        super(Command, self).add_arguments(parser)
        parser.add_argument('--status', '-s', action='store', default=None,
                            choices=['unapproved'],
                            help=u'Status of the downloaded translations')

    def handle_language(self, lang, keyset, path):
        if keyset not in self.tanker.list():
            return

        print(smart_str(
            u'Downloading %s[%s] to: %s' % tuple(map(force_text, (keyset, lang, path)))))

        content = self.tanker.download(
            keyset, lang,
            status=self.options['status'],
            key_not_language=self.options['key_not_language'],
            ref=self.options['branch']
        )

        if not self.options['dry_run']:
            with open(path, 'w') as outp:
                outp.write(smart_str(content))
