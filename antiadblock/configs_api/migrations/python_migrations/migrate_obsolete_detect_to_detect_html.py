# coding=utf-8
import argparse
import logging
from collections import OrderedDict
from sqlalchemy import create_engine


def dict2html(d, root_node=None):
    assert isinstance(d, (dict, list))
    if root_node is None:
        if isinstance(d, dict):
            return "".join([dict2html(value, root_node=key) for key, value in d.iteritems()])
        if isinstance(d, list):
            return "".join([dict2html(value) for value in d])
    assert isinstance(root_node, basestring)

    children = []
    attr = OrderedDict()

    if isinstance(d, dict):
        for key, value in d.iteritems():
            if isinstance(value, dict):
                children.append(dict2html(value, key))
            elif isinstance(value, list):
                children.append(dict2html(value))
            elif isinstance(value, basestring):
                attr[key] = value
            else:
                raise ValueError(u"Unable to serialize type '{}' for key '{}' to html".format(type(value), key))
    elif isinstance(d, list):
        for value in d:
            children.append(dict2html(value))

    if isinstance(d, list):
        xml = u'{content}'
    elif len(children):
        xml = u'<{root} {attributes}>{content}</{root}>'
    else:
        xml = u'<{root} {attributes}></{root}>'

    attributes = u' '.join([u'{key}="{value}"'.format(key=key, value=value) for key, value in attr.iteritems()])
    content = u''.join(children)

    return xml.format(content=content, attributes=attributes, root=root_node)


def create_detect_divs(partner_attrs, common_attrs):

    merged = dict()
    for key in sorted(set(partner_attrs.keys() + common_attrs.keys())):  # sorted hereinafter used for deterministic output
        merged[key] = sorted(list(set(partner_attrs.get(key, list()) + common_attrs.get(key, list()))))

    max_value_len = max(map(len, merged.itervalues()) or [0])

    list_of_dicts = list()
    for i in range(max_value_len):
        my_dict = dict()
        for k, v in merged.iteritems():
            if len(v) > i:
                my_dict[k] = v[i]
        list_of_dicts.append(my_dict)

    return list_of_dicts


def migrate(conn):
    tr = None
    try:
        tr = conn.execution_options(isolation_level="SERIALIZABLE").begin()
        query_update = ''
        pattern_update = u"UPDATE configs SET data = jsonb_set(data, '{{DETECT_HTML}}', jsonb_build_array({})) where id = {};"
        query = "SELECT id, data -> 'DETECT_ELEMS', data -> 'DETECT_CUSTOM', data->'DETECT_HTML' FROM configs WHERE data -> 'DETECT_ELEMS' != '{}'::jsonb OR data -> 'DETECT_CUSTOM' != '[]'::jsonb;"
        for config in conn.execute(query):
            id = config[0]
            detectElems = config[1] or {}
            detectCustom = config[2] or []
            detectHtml = config[3] or []
            detectHtml = [el.replace("'", '"') for el in detectHtml]
            if detectElems:
                detectHtml.extend([dict2html(el, root_node='div') for el in create_detect_divs(detectElems, {})])
            if detectCustom:
                detectHtml.extend([dict2html(el) for el in detectCustom])
            query_update += pattern_update.format(u', '.join(["'" + el + "'" for el in detectHtml]), id)
        if query_update:
            query_update += "UPDATE configs SET data = jsonb_set(data, '{DETECT_CUSTOM}', '[]'::jsonb);"
            query_update += "UPDATE configs SET data = jsonb_set(data, '{DETECT_ELEMS}', '{}'::jsonb);"
            conn.execute(query_update)

        tr.commit()
    except Exception as e:
        logging.log(logging.FATAL, "Произошла ошибка во время выполнения запроса")
        if tr is not None:
            tr.rollback()
        raise e


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--database_uri", action="store", default="postgresql+psycopg2://antiadb:postgres@localhost/configs")
    parser.add_argument("-l", "--log", action="store", default=None)
    args = parser.parse_args()

    logging.basicConfig(filename=args.log, level=logging.INFO,
                        format='[%(asctime)s] %(levelname)s %(message)s', datefmt='%Y.%m.%d %H:%M:%S')

    logging.getLogger('sqlalchemy.engine').setLevel(logging.INFO)

    engine = create_engine(args.database_uri, echo=False)
    with engine.connect() as conn:
        migrate(conn)
    engine.dispose()


if __name__ == "__main__":
    main()
