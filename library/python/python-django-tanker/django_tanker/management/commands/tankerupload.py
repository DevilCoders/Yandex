# coding: utf-8
from functools import partial

from django.utils.encoding import force_text, smart_str

from django_tanker.management.commands import DownloadUploadBaseCommand


class Command(DownloadUploadBaseCommand):
    help = u'Uploads *.po files to the Tanker instance'

    def add_arguments(self, parser):
        super(Command, self).add_arguments(parser)
        parser.add_argument('--mode', '-m', action='store', default=None,
                            choices=['replace', 'update', 'merge'],
                            help=u'Tanker uploading mode'),

    def handle_language(self, lang, keyset, path):
        print(smart_str(u'Uploading %s[%s]: %s' % tuple(map(force_text, (keyset, lang, path)))))

        if keyset not in self.tanker.list():
            handler = self.tanker.create
            print(smart_str(u'Creating keyset: %s' % force_text(keyset)))
        else:
            handler = partial(self.tanker.upload, mode=self.options['mode'],
                              branch=self.options['branch'])

        handler(
            keyset, lang, path,
            key_not_language=self.options['key_not_language'],
            branch=self.options['branch']
        )
