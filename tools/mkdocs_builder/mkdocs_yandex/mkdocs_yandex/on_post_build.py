from mkdocs_yandex.html_postprocessor import HtmlPostProcessor
from mkdocs_yandex.single_page_processor import SinglePageProcessor

from mkdocs_yandex.loggers import get_warning_counts


def do(config):
    if config['yfm20']:
        html_postprocessor = HtmlPostProcessor()
        html_postprocessor.run()

        if config['single_page']:
            exclude_pages = config['single_pages_exclude']
            single_page_processor = SinglePageProcessor()
            single_page_processor.run(exclude_pages=exclude_pages)

    if config['full_strict'] and get_warning_counts():
        raise SystemExit("\nExited with {} warnings in 'full_strict' mode.".format(get_warning_counts()))
