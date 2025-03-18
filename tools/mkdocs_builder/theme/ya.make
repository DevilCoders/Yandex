PY23_LIBRARY()

OWNER(
    g:locdoc
    g:yatool
    workfork
)

PY_SRCS(
    NAMESPACE mkdocs.themes.material_yandex
    __init__.py
)

PEERDIR(
    contrib/python/mkdocs-material
)

RESOURCE_FILES(
    PREFIX mkdocs_theme/
    material_yandex.dist-info/METADATA
    material_yandex.dist-info/entry_points.txt
)

RESOURCE_FILES(
    PREFIX mkdocs/themes/material_yandex/
    main.html
    metrika.html
    mkdocs_theme.yml
    assets/images/ya_favicon.png
    css/extra.css
    partials/footer.html
    partials/header.html
)

END()
