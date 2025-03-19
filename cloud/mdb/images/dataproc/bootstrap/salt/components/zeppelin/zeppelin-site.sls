{% set config_site = {} %}

{% set has_hive = 'hive' in salt['pillar.get']('data:services', []) %}

{% do config_site.update({'zeppelin': salt['grains.filter_by']({
    'Debian': {
        'zeppelin.server.addr': '0.0.0.0',
        'zeppelin.server.port': '8890',
        'zeppelin.server.context.path': '/',
        'zeppelin.interpreter.connect.timeout': '180000',
        'zeppelin.interpreter.output.limit': '524288',
        'zeppelin.interpreter.dir': 'interpreter',
        'zeppelin.interpreter.localRepo': 'local-repo',
        'zeppelin.dep.localrepo': 'local-repo',
        'zeppelin.helium.node.installer.url': 'https://nodejs.org/dist/',
        'zeppelin.helium.npm.installer.url': 'http://registry.npmjs.org/',
        'zeppelin.helium.yarnpkg.installer.url': 'https://github.com/yarnpkg/yarn/releases/download/',
        'zeppelin.server.allowed.origins': '*',
        'zeppelin.spark.useHiveContext': 'true' if has_hive else 'false',
        'zeppelin.notebook.s3.endpoint': 'storage.yandexcloud.net',
    },
}, merge=salt['pillar.get']('data:properties:zeppelin'))}) %}
