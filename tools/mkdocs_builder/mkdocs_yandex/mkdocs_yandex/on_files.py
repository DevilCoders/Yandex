import os

from mkdocs_yandex import run_context
from mkdocs_yandex.loggers import get_logger
from mkdocs.structure import files as mkdocs_files, nav as mkdocs_nav
from mkdocs import utils as mkdocs_utils

logger = get_logger(__name__)


def do(files, config):
    actual, _ = get_files(config, files)
    if actual:
        files = mkdocs_files.Files(actual)

    run_context.files = files
    return files


def get_files(config, files=None):
    # mkdocs 1.0.4 builds not only config['nav'] but everything in docs_dir,
    # so - it should be properly filled or be entirely buildable

    actual, excessive = [], []

    if config.get('extra', {}).get('build_only_toc', False):
        if not files:
            files = mkdocs_files.get_files(config)

        nav_config = config['nav'] or mkdocs_utils.nest_paths(f.src_path for f in files.documentation_pages())
        mkdocs_nav._data_to_navigation(nav_config, files, config)

        for file in files:
            # 'includes' dir (with no underscore) is still used in almost deprecated yql docs (see https://st.yandex-team.ru/YQL-9740)
            if file.is_documentation_page() and file.page is None and os.path.basename(os.path.dirname(file.src_path)) not in ('includes', '_includes'):
                logger.debug('File %s won\'t be processed in build_only_toc mode', file.src_path)
                excessive.append(file)
            else:
                actual.append(file)

    return actual, excessive
