PY23_LIBRARY()

OWNER(
    g:locdoc
    workfork
)

PY_SRCS(
    TOP_LEVEL
    mkdocs_yandex/ext/__init__.py
    mkdocs_yandex/ext/jinja2/__init__.py
    mkdocs_yandex/ext/jinja2/cut.py
    mkdocs_yandex/ext/jinja2/inc.py
    mkdocs_yandex/ext/jinja2/list.py
    mkdocs_yandex/ext/jinja2/note.py
    mkdocs_yandex/ext/markdown/__init__.py
    mkdocs_yandex/ext/markdown/anchor.py
    mkdocs_yandex/ext/markdown/attr_list.py
    mkdocs_yandex/ext/markdown/cut.py
    mkdocs_yandex/ext/markdown/full_strict.py
    mkdocs_yandex/ext/markdown/inc.py
    mkdocs_yandex/ext/markdown/indent.py
    mkdocs_yandex/ext/markdown/link.py
    mkdocs_yandex/ext/markdown/list.py
    mkdocs_yandex/ext/markdown/note.py
    mkdocs_yandex/ext/markdown/pymdownx/__init__.py
    mkdocs_yandex/ext/markdown/pymdownx/extrarawhtml.py
    mkdocs_yandex/ext/markdown/pymdownx/highlight.py
    mkdocs_yandex/ext/markdown/pymdownx/superfences.py
    mkdocs_yandex/ext/markdown/pymdownx/tasklist.py
    mkdocs_yandex/ext/markdown/pymdownx/tilde.py
    mkdocs_yandex/ext/markdown/pymdownx/util.py
    mkdocs_yandex/ext/markdown/slugs.py
    mkdocs_yandex/artefact.py
    mkdocs_yandex/html_postprocessor.py
    mkdocs_yandex/loggers.py
    mkdocs_yandex/nav.py
    mkdocs_yandex/on_config.py
    mkdocs_yandex/on_env.py
    mkdocs_yandex/on_files.py
    mkdocs_yandex/on_nav.py
    mkdocs_yandex/on_page_markdown.py
    mkdocs_yandex/on_post_build.py
    mkdocs_yandex/plugin.py
    mkdocs_yandex/regex.py
    mkdocs_yandex/run_context.py
    mkdocs_yandex/single_page_processor.py
    mkdocs_yandex/util.py
)

PEERDIR(
    contrib/python/beautifulsoup4
    contrib/python/Jinja2
    contrib/python/lxml
    contrib/python/mkdocs
    contrib/python/Markdown
    contrib/python/python-slugify
)

RESOURCE_FILES(
    PREFIX mkdocs_plugin/
    mkdocs_yandex.dist-info/entry_points.txt
    mkdocs_yandex.dist-info/METADATA
)

END()

RECURSE_FOR_TESTS(
    tests
)
