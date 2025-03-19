"""
HTTP response formatters.
"""

from xml.etree import ElementTree as etree

from flask import Response, jsonify


def json_formatter(status, display_name=None, scope=None, **kwargs):
    """
    Format response in JSON format.
    """
    response = {
        'status': {
            'value': status,
        },
    }

    if scope:
        response['oauth'] = dict(scope=scope)

    if display_name:
        response['display_name'] = dict(name=display_name)

    response.update(kwargs)

    return jsonify(response)


def xml_formatter(login, display_name=None, scope=None, **kwargs):
    """
    Format response in XML format.
    """
    doc = etree.Element('doc')

    if scope:
        oauth = etree.SubElement(doc, 'OAuth')
        etree.SubElement(oauth, 'scope').text = scope

    etree.SubElement(doc, 'login').text = login

    if display_name:
        node = etree.SubElement(doc, 'display_name')
        etree.SubElement(node, 'name').text = display_name

    for key, value in kwargs.items():
        etree.SubElement(doc, key).text = value

    return Response(
        etree.tostring(doc, encoding='unicode'), mimetype='application/xml')
