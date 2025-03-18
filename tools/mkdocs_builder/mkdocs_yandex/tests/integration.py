import logging
import os
import pytest
import re

from mkdocs import config
from mkdocs import utils
from mkdocs.commands import build
import yatest.common as yac


logger = logging.getLogger(__name__)

PROJECTS_ROOT = yac.source_path('locdoc/doc_tools/mkdocs/test-doc/')
SMALL_PROJECTS_ROOT = yac.source_path('tools/mkdocs_builder/mkdocs_yandex/tests/data')


@pytest.mark.parametrize('docs_name', ['app_host', 'test'])
def test_stright_forward(docs_name):
    root_dir = os.path.join(PROJECTS_ROOT, docs_name)
    docs_dir = os.path.join(root_dir, 'docs')

    output_dir = yac.output_path(docs_name)
    cwd = yac.work_path(docs_name)
    os.mkdir(cwd)

    cmd = [
        yac.binary_path('tools/mkdocs_builder/mkdocs_builder'),
        '--config', os.path.join(root_dir, 'mkdocs.yml'),
        '--output-dir', output_dir,
        '--docs-dir', docs_dir,
        '--verbose',
    ]
    yac.execute(cmd, cwd=cwd, check_exit_code=True)


def test_build_anchor():
    root_dir = os.path.join(SMALL_PROJECTS_ROOT, 'anchor')

    output_dir = yac.output_path('anchor')

    build.build(config.load_config(
        docs_dir=os.path.join(root_dir, 'docs'),
        config_file=os.path.join(root_dir, 'mkdocs.yml'),
        strict=True,
        site_dir=output_dir,
        use_directory_urls=False,
    ))

    page_path = os.path.join(output_dir, 'index.html')
    return yac.canonical_file(_fix_doccenter_copyright(page_path), local=True)


def test_build_anchor_dir_urls():
    root_dir = os.path.join(SMALL_PROJECTS_ROOT, 'anchor')

    output_dir = yac.output_path('anchor_use_directory_urls')

    build.build(config.load_config(
        docs_dir=os.path.join(root_dir, 'docs'),
        config_file=os.path.join(root_dir, 'mkdocs.yml'),
        strict=True,
        site_dir=output_dir,
        use_directory_urls=True,
        theme='mkdocs',
    ))

    return yac.canonical_file(os.path.join(output_dir, 'index.html'), local=True)


def test_build_variables():
    root_dir = os.path.join(SMALL_PROJECTS_ROOT, 'variables')
    output_dir = yac.output_path('variables')

    build.build(config.load_config(
        docs_dir=os.path.join(root_dir, 'docs'),
        config_file=os.path.join(root_dir, 'mkdocs.yml'),
        strict=True,
        site_dir=output_dir,
    ))

    with open(os.path.join(output_dir, 'index.html'), 'r') as f:
        content = f.read()
    assert 'This should appear in result' in content
    assert 'And this should not' not in content


def test_singlepage_themes():
    root_dir = os.path.join(SMALL_PROJECTS_ROOT, 'variables')

    for theme in utils.get_theme_names():
        output_dir = yac.output_path('variables_' + theme)

        build.build(config.load_config(
            docs_dir=os.path.join(root_dir, 'docs'),
            config_file=os.path.join(root_dir, 'mkdocs.yml'),
            strict=True,
            site_dir=output_dir,
            theme=theme,
            plugins=[dict(yandex=dict(single_page=True, yfm20=True))],
            use_directory_urls=False,
        ))

        for page in ['index.html', 'single/index.html']:
            logger.info('Checking page %s for theme %s', page, theme)
            with open(os.path.join(output_dir, page), 'r') as f:
                content = f.read()
            assert 'This should appear in result' in content
            assert 'And this should not' not in content


def test_full_strict():
    root_dir = os.path.join(SMALL_PROJECTS_ROOT, 'full_strict')

    output_dir = yac.output_path('full_strict')

    try:
        build.build(config.load_config(
            docs_dir=os.path.join(root_dir, 'docs'),
            config_file=os.path.join(root_dir, 'mkdocs.yml'),
            strict=True,
            site_dir=output_dir,
        ))
    except SystemExit as e:
        assert "Exited with 2 warnings in 'full_strict' mode." in e.args[0]


_COPYRIGHT_RE = re.compile(r'(Copyright Yandex) \d{4}', re.UNICODE)


def _fix_doccenter_copyright(page_path):
    with open(page_path, 'r') as f:
        content = f.read()

    content = _COPYRIGHT_RE.sub(r'\1', content)

    with open(page_path, 'w') as f:
        f.write(content)

    return page_path
