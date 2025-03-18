import os
import tarfile

from six.moves.urllib.parse import urlparse

from mkdocs_yandex import run_context


class Artefact:

    def create_tar(self):
        dir_from = run_context.config['site_dir']
        u = urlparse(run_context.config['site_url'])
        lang = 'ru' if u.netloc.endswith('.ru') else 'en'  # default language
        if 'extra' in run_context.config and 'language' in run_context.config['extra']:
            lang = run_context.config['extra']['language']
        arc_inner_path = '/'.join([u.netloc, lang, u.path.strip('/'), ''])
        arc_path = os.path.join(os.path.dirname(dir_from), 'deploy.tar.gz')
        with tarfile.open(arc_path, "w:gz") as tar:
            tar.add(dir_from, arcname=arc_inner_path)
