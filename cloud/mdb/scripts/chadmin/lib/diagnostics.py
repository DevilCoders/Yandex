import io
import json
import gzip
import yaml

from .utils import execute, upload_to_paste, upload_to_sandbox


def collect_diagnostics(host, *args, format, use_pssh, sudo):
    command = f'ch-diagnostics --format {format}.gz'
    if sudo:
        command = f'sudo {command}'
    command = ' '.join((command, *args))
    data = execute(host=host, command=command, use_pssh=use_pssh, binary=True)
    with gzip.GzipFile(mode='rb', fileobj=io.BytesIO(data)) as f:
        return f.read().decode()


def format_diagnostics_data(host, data):
    data = yaml.safe_load(data)

    buffer = io.StringIO()

    _write_title(buffer, f'Diagnostics data for host {host}')
    for section in data:
        section_name = section['section']
        if section_name:
            _write_subtitle(buffer, section_name)

        for name, item in section['data'].items():
            if item['type'] == 'string':
                _write_string_item(buffer, name, item)
            elif item['type'] == 'url':
                _write_url_item(buffer, name, item)
            elif item['type'] == 'query':
                _write_query_item(buffer, section_name, name, item)
            elif item['type'] == 'command':
                _write_command_item(buffer, section_name, name, item)
            elif item['type'] == 'xml':
                _write_xml_item(buffer, section_name, name, item)
            else:
                _write_unknown_item(buffer, section_name, name, item)

    return buffer.getvalue()


def _write_title(buffer, value):
    buffer.write(f'===+ {value}\n')


def _write_subtitle(buffer, value):
    buffer.write(f'====+ {value}\n')


def _write_string_item(buffer, name, item):
    value = item['value']
    if value:
        value = f'**{value}**'
    buffer.write(f'{name}: {value}\n')


def _write_url_item(buffer, name, item):
    value = item['value']
    buffer.write(f'**{name}**\n{value}\n')


def _write_xml_item(buffer, section_name, name, item):
    if section_name:
        buffer.write(f'=====+ {name}\n')
    else:
        _write_subtitle(buffer, name)

    _write_result(buffer, item['value'], format='XML')


def _write_query_item(buffer, section_name, name, item):
    if section_name:
        buffer.write(f'=====+ {name}\n')
    else:
        _write_subtitle(buffer, name)

    _write_query(buffer, item['query'])
    _write_result(buffer, item['result'])


def _write_command_item(buffer, section_name, name, item):
    if section_name:
        buffer.write(f'=====+ {name}\n')
    else:
        _write_subtitle(buffer, name)

    _write_command(buffer, item['command'])
    _write_result(buffer, item['result'])


def _write_unknown_item(buffer, section_name, name, item):
    if section_name:
        buffer.write(f'**{name}**\n')
    else:
        _write_subtitle(buffer, name)

    json.dump(item, buffer, indent=2)


def _write_query(buffer, query):
    buffer.write('<{ query\n')
    buffer.write('%%(SQL)\n')
    buffer.write(query)
    buffer.write('\n%%\n')
    buffer.write('}>\n\n')


def _write_command(buffer, command):
    buffer.write('<{ command\n')
    buffer.write('%%\n')
    buffer.write(command)
    buffer.write('\n%%\n')
    buffer.write('}>\n\n')


def _write_result(buffer, result, format=None):
    if len(result) > 4 * 1024**2:
        buffer.write(upload_to_sandbox(result))
        buffer.write('\n')
    elif len(result) > 40 * 1024:
        try:
            link = upload_to_paste(result)
        except Exception:
            link = upload_to_sandbox(result)
        buffer.write(link)
        buffer.write('\n')
    else:
        buffer.write(f'%%({format})\n' if format else '%%\n')
        buffer.write(result)
        buffer.write('\n%%\n')
