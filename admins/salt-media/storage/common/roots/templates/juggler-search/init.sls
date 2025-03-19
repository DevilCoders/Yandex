{% set juggler_client_version = pillar.get('juggler_client_version', '2.3.2104141650') %}
{% set monrun_version = pillar.get('monrun_version', '1.3.5') %}

{% if juggler_client_version != "ignore-juggler-client-version"%}
/etc/yandex/juggler-client-media.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - contents: |
        juggler_client_version='2'
        juggler_installation='search'

juggler-client:
  pkg.installed:
    - refresh: True
    - version: {{juggler_client_version}}
{% endif %}

{% if monrun_version != "ignore-monrun-version"%}
monrun:
  pkg.installed:
    - refresh: True
    - version: {{monrun_version}}
{% endif %}

/etc/yandex-pkgver-ignore.d/juggler-search-common-template:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - contents: |
        juggler-client
        monrun
