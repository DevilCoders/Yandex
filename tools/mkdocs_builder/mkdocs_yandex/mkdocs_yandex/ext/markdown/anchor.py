import os

from markdown import Extension

from markdown.treeprocessors import Treeprocessor

from mkdocs_yandex import run_context
from mkdocs_yandex.loggers import get_logger
from mkdocs_yandex.util import strip_html

ID_SEP = '__'


def makeExtension(**kwargs):
    return AnchorExtension(**kwargs)


class AnchorTreeProcessor(Treeprocessor):
    log = get_logger(__name__)

    def run(self, doc):
        page_path = False
        for el in doc.iter():
            attr_id = el.get('id')
            data = {
                'tag': el.tag,
                'text': None,
                'uid': self.get_uid(run_context.page_src_path)
            }
            if el.tag in ['h1', 'h2', 'h3', 'h4', 'h5', 'h6']:
                path = run_context.files.get_file_from_path(run_context.page_src_path).abs_dest_path
                data['text'] = strip_html(el)
                if not page_path:
                    page_path = path
                    run_context.anchors[path] = data
            if attr_id:
                run_context.known_ids.append(attr_id)
                uid = self.get_uid(run_context.page_src_path, attr_id)
                data['uid'] = uid
                el.set('data-uid', uid)
                path = page_path + '#' + attr_id
                if path in run_context.anchors:
                    # Duplicate within page
                    self.log.warn('Duplicate id "{}" in {}'.format(attr_id, run_context.page_src_path))
                else:
                    run_context.anchors[path] = data

    def get_uid(self, src_path, id=None):
        # TODO: Probably we don't need ufix for unique ids
        base = os.path.splitext(src_path)[0]
        ufix = base.replace('/', ID_SEP).replace('\\', ID_SEP)
        uid = ID_SEP.join([ufix, id]) if id else ufix
        return uid


class AnchorExtension(Extension):

    def extendMarkdown(self, md):
        anchor_treeprocessor = AnchorTreeProcessor(md)
        md.treeprocessors._sort()
        priority = md.treeprocessors._priority[-1].priority - 5
        md.treeprocessors.register(anchor_treeprocessor, "anchor_treeprocessor", priority)
