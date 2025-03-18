import json
import os

from lxml import etree
from six.moves.urllib.parse import urlparse

from mkdocs.commands.build import get_context
from mkdocs.structure.files import File
from mkdocs.structure.pages import Page
from mkdocs.utils import write_file

from mkdocs_yandex import run_context
from mkdocs_yandex.loggers import get_logger

log = get_logger(__name__)


class SinglePageProcessorError(Exception):
    pass


class SinglePageProcessor(object):

    def run(self, exclude_pages=None):
        log.info("Creating a single-page document")
        self.single_page_dir = os.path.join(run_context.config['site_dir'], 'single')
        self.single_page_file = os.path.join(self.single_page_dir, 'index.html')
        self.exclude_pages = exclude_pages if exclude_pages else []
        self.process()

    def _check_page_on_exclude(self, page):
        """ Exclude all child pages by section title

        ```yml
        -  'Section.title': # == Page1.parent.title == Page2.parent.title
          - 'Page1.title': 'Page1.file.src_path'
          - 'Page2.title': 'Page2.file.src_path'
          ...
        ```

        """
        if page.title in self.exclude_pages:
            return True
        if page.parent:
            return self._check_page_on_exclude(page.parent)
        return False

    def process(self):

        root_el = etree.Element('div')
        root_el.set('class', 'single-page')
        for page in run_context.nav.pages:
            if self._check_page_on_exclude(page):
                log.debug('Exclude page "%s" from single page', page.title)
                continue
            path = os.path.join(run_context.config['site_dir'], page.url)
            page_root_el = run_context.etrees[path]
            page_content_el = page_root_el.xpath('.//article | .//div[@role="main"]')
            if page_content_el:
                page_content_el = page_content_el[0]

            if page_content_el is None or len(page_content_el) == 0:
                log.info(etree.tostring(page_root_el))
                raise SinglePageProcessorError('Failed to determine content node for path "{}". Probably single page processor is not adapted to the used theme.'.format(page.url))

            attrs = page_content_el.attrib
            attrs['class'] = 'chunk'

            self.process_chunk(page_content_el, path)

            root_el.append(page_content_el)

        content = etree.tostring(root_el, encoding='utf8', method='html').decode('utf-8')
        html = self.render(content)
        self.process_menu(html)

    def process_menu(self, html):
        menu_path = os.path.join(run_context.config['site_dir'], 'menu.json')

        # Skip if no menu.json.
        if not os.path.isfile(menu_path):
            return

        # This is for mkdocs-doccenter theme
        u = urlparse(run_context.config['site_url'])
        extra_prefix = u.path.strip('/')

        def traverse(obj, path=None):
            if path is None:
                path = []
            if isinstance(obj, dict):
                value = {k: traverse(v, path + [k]) for k, v in obj.items()}
            elif isinstance(obj, list):
                value = [traverse(elem, path + [[]]) for elem in obj]
            else:
                value = obj
                if path[-1] in ['path', 'index']:
                    href = obj
                    href_new = href[len(extra_prefix)+1:] if extra_prefix else href
                    anchor = os.path.join(run_context.config['site_dir'], href_new)
                    attr_id = run_context.anchors.get(anchor, {}).get('uid')
                    if attr_id:
                        value = '#'.join([extra_prefix + '/single/index.html', attr_id])
            return value

        with open(menu_path) as f:
            data = json.load(f)
            menu_single = traverse(data)
            menu_single_json = json.dumps(menu_single, indent=2, ensure_ascii=False).encode('utf8')
            with open(os.path.join(self.single_page_dir, 'menu.json'), "wb") as fo:
                fo.write(menu_single_json)

    def process_chunk(self, root, path):

        nav = root.find('.//nav')
        if nav is not None:
            nav.getparent().remove(nav)

        for el in root.iter():
            id = el.get('id')
            uid = el.get('data-uid')

            if id and uid:
                el.set('id', uid)
                del (el.attrib['data-uid'])

            if el.tag == 'a':
                href = el.get('href')
                uid = self.get_uid(path, href)
                if uid:
                    el.set('href', '#' + uid)

            if el.tag == 'img':
                src = el.get('src')
                abs_path = os.path.normpath(os.path.join(os.path.dirname(path), src))
                rel_path = os.path.relpath(abs_path, self.single_page_dir)
                el.set('src', rel_path)

    def patch_nav(self, html):
        parser = etree.HTMLParser(encoding='utf-8')
        tree = etree.HTML(html, parser=parser)
        nav_link_els = tree.xpath('.//nav//a')

        for a_el in nav_link_els:
            href = a_el.get('href')
            up_href = href[3:]
            x = os.path.join(run_context.config['site_dir'], 'foo')
            uid = self.get_uid(x, up_href)
            if uid:
                a_el.set('href', '#' + uid)
        return etree.tostring(tree, encoding='utf8', method='html')
        # ---

    def render(self, content):
        # fake
        the_file = File('single/index.md', run_context.config['docs_dir'],
                        run_context.config['site_dir'], run_context.config['use_directory_urls'])
        page = Page('Single page', the_file, run_context.config)
        page.content = content

        context = get_context(run_context.nav, run_context.files, run_context.config)
        template = run_context.env.get_template('main.html')
        output = template.render(context, config=run_context.config, page=page)
        output = self.patch_nav(output)
        write_file(output, os.path.join(run_context.config['site_dir'], 'single/index.html'))
        return output

    def get_uid(self, path, href):
        scheme, netloc, url, params, query, fragment = urlparse(href)

        if not (scheme or os.path.isabs(href)):
            if fragment and not url:
                anchor = "#".join([path, fragment])
            else:
                anchor = os.path.normpath(os.path.join(os.path.dirname(path), href))
            uid = run_context.anchors[anchor]['uid'] if anchor in run_context.anchors else None
            return uid
