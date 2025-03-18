import copy
import shlex


def get_params(string):
    pairs = shlex.split(string)
    params = dict()
    for pair in pairs:
        item = pair.split('=')
        params[item[0]] = item[1].strip()
    return params


def is_int_positive(str):
    res = True
    try:
        val = int(str)
        if val < 0:
            res = False
    except ValueError:
        res = False
    return res


def strip_html(element):
    el = copy.copy(element)
    for e in el.iter():
        if e.tag == 'a' and 'headerlink' in e.get('class', []):
            el.remove(e)
    if len(el) == 0:
        return el.text
    return ''.join(el.itertext())


def copy_etree_element(elem):
    if hasattr(elem, 'copy'):
        return elem.copy()

    newelem = elem.makeelement(elem.tag, elem.attrib)
    newelem.text = elem.text
    newelem.tail = elem.tail
    newelem[:] = elem
    return newelem
