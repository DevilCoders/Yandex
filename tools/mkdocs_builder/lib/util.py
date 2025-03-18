import contextlib
import errno
import io
import logging
import os
import shutil
import six
import tempfile
import typing as tp  # noqa

logger = logging.getLogger(__name__)


def fix_permissions(folder_path):
    for root, dirs, files in os.walk(folder_path):
        for f in files:
            abs_path = os.path.join(root, f)
            os.chmod(abs_path, 0o644)
        for d in dirs:
            abs_path = os.path.join(root, d)
            os.chmod(abs_path, 0o744)
    os.chmod(folder_path, 0o744)


@contextlib.contextmanager
def temp_dir(suffix=''):
    path = tempfile.mkdtemp(suffix=suffix, dir=os.environ['TMPDIR'])
    yield path
    shutil.rmtree(path, ignore_errors=True)


@contextlib.contextmanager
def custom_env(env):
    old = os.environ.copy()
    try:
        os.environ.update(env)
        yield env
    finally:
        os.environ = old


def merge_content(
        docs_dir,  # type: six.string_types
        nav_tree,  # type: tp.List[tp.Dict[six.string_types, six.string_types]]
        titles_list,  # type: tp.List[six.string_types]
        dst,  # type: tp.List[six.string_types]
        need_anchor=True,  # type: bool
        ):
    # type: (...) -> tp.List[six.string_types]
    """
    Merge content of all found md files
    :param docs_dir: full path to docs directory
    :param nav_tree: tree in mkdocs nav style
                            https://www.mkdocs.org/user-guide/writing-your-docs/#configure-pages-and-navigation
    :param titles_list: list of hierarchy titles
    :param dst: intermediate list for tree traversal
    :param need_anchor: True if should insert additional anchors for sections
    :return: list of components to build resulted md file
    """
    logger.debug(u'Copying single page content for {}'.format(u'->'.join(titles_list)))

    from . import process

    if isinstance(nav_tree, six.string_types):
        abs_path = os.path.join(docs_dir, nav_tree)
        with io.open(abs_path, 'r', encoding='utf-8') as f:
            content = f.read()
            top_anchor = nav_tree.replace(u'.md', u'')
            dst += [
                u'<h2 id={} class=single>{}</h2>'.format(top_anchor, u" > ".join(titles_list)),
                process.preprocess_markdown_for_single_page(top_anchor, content, need_anchor)
            ]

    if isinstance(nav_tree, list):
        for page in nav_tree:
            element_title, filename = next(iter(page.items()))
            updated_titles = titles_list + [element_title]
            dst = merge_content(docs_dir, filename, updated_titles, dst, need_anchor)

    return dst


def ensure_dir(path):
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno != errno.EEXIST or not os.path.isdir(path):
            raise


def is_yfm_enabled(cfg, option=None):
    return 'yandex' in dict(cfg['plugins']) and (option is None or dict(cfg['plugins'])['yandex'].config[option])
