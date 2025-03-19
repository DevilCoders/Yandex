import ruamel.yaml as yaml
import ruamel.yaml.compat as ryc
import ruamel.yaml.scalarstring as rys

import django.template
import django.utils.safestring as dus


register = django.template.Library()

GORE_TO_DB_NAME = {
    'mdb-postgres': 'postgresql',
    'mdb-elasticsearch': 'elasticsearch',
    'mdb-mysql': 'mysql',
    'mdb-greenplum': 'greenplum',
    'mdb-clickhouse': 'clickhouse',
    'mdb-mongodb': 'mongodb',
    'mdb-redis': 'redis',
    'mdb-sqlserver': 'sqlserver',
    'mdb-core': 'core',
}

GORE_NAMES = {
    'mdb-postgres': 'PostgreSQL',
    'mdb-elasticsearch': 'ElasticSearch',
    'mdb-mysql': 'MySQL',
    'mdb-greenplum': 'Greenplum',
    'mdb-clickhouse': 'ClickHouse',
    'mdb-mongodb': 'MongoDB',
    'mdb-redis': 'Redis',
    'mdb-sqlserver': 'SQL Server',
    'mdb-core': 'Core'
}


@register.filter
def gore_name(value):
    return GORE_NAMES.get(value, value)


@register.simple_tag(takes_context=True)
def gore_image(context, group_name):
    result = ''
    static_address = context.get('static_address')
    if group_name and static_address:
        if group_name in GORE_TO_DB_NAME:
            value = GORE_TO_DB_NAME[group_name]
            result = dus.mark_safe(
                f'<img title="{value}" src="{static_address}/images/{value}.png"></img>'
            )
    return result


@register.filter
def vector_item_to_yaml(yaml_data):
    if yaml_data['type'] == 'remap':
        yaml_data['source'] = rys.PreservedScalarString(yaml_data['source'])
    elif yaml_data['type'] == 'filter':
        yaml_data['condition'] = rys.PreservedScalarString(yaml_data['condition'])

    stream = ryc.StringIO()
    yaml.YAML().dump(yaml_data, stream)
    return stream.getvalue()
