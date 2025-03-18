import os
import re

from lxml import etree
from six.moves.urllib.parse import urlparse, urlunparse

from mkdocs_yandex.regex import RE_LINK_TEXT_WITH_OPTS
from mkdocs_yandex.loggers import get_logger
from mkdocs_yandex import run_context
from mkdocs_yandex.ext.markdown import indent as indent_ext

try:
    import HTMLParser

    def unescape(*args, **kwargs):
        HTMLParser().unescape(*args, **kwargs)
except:
    from html.parser import HTMLParser
    from html import unescape


log = get_logger(__name__)


# TODO: Check headings: only one h1 (should be first).
#       Check difference between adjacent levels.
#       Should we automatically adjust levels?


class HtmlPostProcessor:
    RE_LINK_TEXT_WITH_OPTS = re.compile(RE_LINK_TEXT_WITH_OPTS)

    LIST_SYNTAX_HINT = 'Mind that first element of outer most list is treated as heading, the content must be placed ' \
                       'after the heading and be {} space indented.'.format(indent_ext.TAB_LENGTH)

    dl_cls = {
        'details': 'cut',
        'dl': '',
        'dropdown': 'dropdown',
        'radio': 'radiobutton',
        'tabs': 'htab',
    }

    material_themes = ['material', 'material_yandex']

    def run(self):
        self.is_material_theme = run_context.config['theme'].name in self.material_themes
        log.info("Processing HTML pages")
        self.process()

    def process(self):

        for mkdocs_file in run_context.files:
            if mkdocs_file.is_documentation_page():
                # in
                with open(mkdocs_file.abs_dest_path, "r") as f:
                    content = f.read()
                    parser = etree.HTMLParser(encoding='utf-8')
                    self.tree = etree.HTML(content, parser=parser)
                    self.process_links(mkdocs_file)
                    self.process_lists(mkdocs_file.dest_path)
                # out
                run_context.etrees[mkdocs_file.abs_dest_path] = self.tree
                output = etree.tostring(self.tree, encoding='utf8', method='html')
                with open(mkdocs_file.abs_dest_path, "wb") as fo:
                    fo.write(output)

    def process_links(self, mkdocs_file):
        for el in self.tree.iter():
            if el.tag == 'a':
                if el.text is None:
                    continue
                text = el.text.strip()
                href = el.get('href')
                opts = ''
                class_attr = el.get('class')
                classes = class_attr.split() if class_attr else []
                if 'xref' not in classes:
                    classes.append('xref')

                m = self.RE_LINK_TEXT_WITH_OPTS.match(text)
                if m:
                    opts = m.group('opts')
                    text = m.group(1).rstrip()
                    el.text = text

                # Replace link text with corresponding (T)itle text.
                if 'T' in opts:
                    anchor_text = get_anchor_text(mkdocs_file.abs_dest_path, href)
                    el.text = anchor_text
                    if anchor_text != text:
                        log.debug('Link text and target heading text aren\'t equal in {}:'
                                  '    "{}" vs "{}"'.format(mkdocs_file.src_path, text, anchor_text))

                # Force (N)ew tab.
                if 'N' in opts:
                    el.set('target', '_blank')

                # (B)utton styled
                if 'B' in opts:
                    classes.append('button')

                # (P)op up
                if 'P' in opts:
                    classes.append('popup-link')

                el.set('class', ' '.join(classes))

    # Lists
    def process_lists(self, file_path):
        tabs_counter = 0
        for elem in self.tree.iter('div'):
            class_attr = elem.get('class', '')
            classes = class_attr.split()
            cls_intersect = set(self.dl_cls.keys()) & set(classes)
            if not cls_intersect:
                continue

            cls = cls_intersect.pop()
            ul_elem = elem[0]
            container = []
            # All special treated lists have default implementation - dl
            morph_tag = 'dl'
            morph_attrs = {'class': 'dl ' + self.dl_cls[class_attr]}

            if self.is_material_theme:
                if cls in ['tabs', 'details']:
                    morph_tag = 'div'
                    if cls == 'tabs':
                        tabs_counter += 1
                        morph_attrs = {'class': 'superfences-tabs'}
                    if cls == 'details':
                        morph_attrs = {'class': 'cut'}

            if elem.get('is-cat'):

                first_elem = elem[0]

                title = elem.get('cut-title')
                log.info(title)
                id = first_elem.get('id')
                uid = first_elem.get('data-uid')

                content = list(elem)
                elem.clear()

                if not self.is_material_theme:
                    classes = class_attr + ' collapsible collapsed' if cls == 'details' else None
                    self.append_dlentry(elem, title, content, id, uid, classes)

                self.append_details(elem, title, content, id, uid)

                self.morph_element(elem, morph_tag, morph_attrs)
            else:
                for i, li_elem in enumerate(ul_elem):

                    if not len(li_elem):
                        log.warning('Seems that your `{%% list %%}` syntax is wrong in %s near\n%s\n%s',
                                    file_path, unescape(etree.tostring(li_elem).decode('utf-8')), self.LIST_SYNTAX_HINT)
                        continue

                    first_elem = li_elem[0]
                    if first_elem.tag != 'p':
                        raise Exception('Something went wrong, please check the `{{% list %}}` syntax near `{}`. {}'
                                        .format(unescape(etree.tostring(li_elem).decode('utf-8')), self.LIST_SYNTAX_HINT))

                    title = first_elem.text
                    id = first_elem.get('id')
                    uid = first_elem.get('data-uid')

                    content = li_elem[1:]

                    if not self.is_material_theme:
                        classes = class_attr + ' collapsible collapsed' if cls == 'details' else None
                        self.append_dlentry(container, title, content, id, uid, classes)

                    elif cls == 'details':
                        self.append_details(container, title, content, id, uid)
                    elif cls in ['dl', 'radio', 'dropdown']:
                        self.append_dlentry(container, title, content, id, uid)
                    elif cls == 'tabs':
                        self.append_tab(container, title, content, tabs_counter, i, id, uid)

                ul_elem.clear()
                ul_elem.extend(container)
                self.morph_element(ul_elem, morph_tag, morph_attrs)

    def append_tab(self, container, title, content, tabs_idx, tab_idx, id=None, uid=None):
        name = '_'.join(['__tabs', str(tabs_idx)])
        input_id = '_'.join([name, str(tab_idx)])
        input_el = etree.Element('input',
                                 {'type': 'radio', 'name': name, 'id': input_id})
        if tab_idx == 0:
            input_el.set('checked', 'checked')
        label_el = etree.Element('label', {'for': input_id})
        label_el.text = title
        if id is not None:
            label_el.set('id', id)
        if uid is not None:
            label_el.set('data-uid', uid)
        div_content_el = etree.Element('div', {'class': 'superfences-content'})
        div_content_el.extend(content)
        container.extend([input_el, label_el, div_content_el])

    def append_details(self, container, title, content, id=None, uid=None):
        details_el = etree.Element('details', {'class': 'tldr'})
        summary_el = etree.Element('summary')
        summary_el.text = title
        if id is not None:
            summary_el.set('id', id)
        if uid is not None:
            summary_el.set('data-uid', uid)
        details_el.append(summary_el)
        details_el.extend(content)
        container.append(details_el)

    def append_dlentry(self, container, title, content, id, uid, classes=None):
        classes = ' ' + classes if classes is not None else ''
        dt_elem = etree.Element('dt', {'class': 'dt dlterm' + classes})
        dt_elem.text = title
        if id is not None:
            dt_elem.set('id', id)
        if uid is not None:
            dt_elem.set('data-uid', uid)
        dd_elem = etree.Element('dd', {'class': 'dd'})
        dd_elem.extend(content)
        container.extend([dt_elem, dd_elem])

    def morph_element(self, elem, tag, attrs):
        elem.tag = tag
        for name in attrs:
            elem.set(name, attrs[name])


# -----------------------------------------------------------------------------


def get_anchor_text(path_from, ref):
    anchor_text = None
    scheme, netloc, path, params, query, fragment = urlparse(ref)

    if scheme or netloc or os.path.isabs(ref):
        return None

    dir_from = os.path.dirname(path_from)

    if not path and fragment:
        abs_path_to = os.path.normpath(os.path.join(dir_from, os.path.basename(path_from)))
    else:
        abs_path_to = os.path.normpath(os.path.join(dir_from, path))

    if run_context.config['use_directory_urls'] and not abs_path_to.endswith('.html'):
        abs_path_to = os.path.join(abs_path_to, 'index.html')
    components = (scheme, netloc, abs_path_to, params, query, fragment)
    anchor = urlunparse(components)
    if anchor in run_context.anchors:
        anchor_text = run_context.anchors[anchor]['text']
    else:
        log.warn('Failed to resolve link %s from page %s (expected anchor was %s)', ref, path_from, anchor)

    return anchor_text
