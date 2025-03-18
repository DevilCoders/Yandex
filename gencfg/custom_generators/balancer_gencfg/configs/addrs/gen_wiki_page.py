#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

import copy
import json


def bold(text):
    return '**' + text + '**' if text else text


def tag(tag):
    return tag if tag == 'stable' else bold(tag)


def all_bold(list):
    return [bold(l) for l in list]


def all_tag(list):
    return [tag(l) for l in list]


# def gen_config_line(default, config):
def gen_wiki_page(input_config, outfile):
    head = ['Название', 'Пример верстки', 'Запрос в тестинг']
    sources_names = ['Визард', 'ППО', 'Геокодер', 'Народная карта', 'Транспорт', 'Остальные']
    sources = ['wizard', 'business', 'geocoder', 'wiki', 'transit', 'other']

    header = all_bold(head + ['Метапоиски'] + sources_names)

    outfile.write('#|\n')
    outfile.write('||' + '|'.join(header) + '||')
    outfile.write('\n')
    default = input_config['default']

    res = {}
    # mapsfinder = re.compile(r'([a-zA-Z0-9]+(\\\\\\\\\.[a-zA-Z0-9]+)?(\.\+[a-zA-Z0-9]+)?)')
    for key, c in input_config.iteritems():

        outline = []
        if key == 'suggest':
            continue
        if key != 'default':
            name = key
            outline.append(name)
            api_name = 'http://addrs-testing.search.yandex.net/search/' + name + '/'
            outline.append(
                '((https://l7test.yandex.ru/maps?host_config[inthosts][search]=' + api_name + ' ' + name + '))')
            outline.append(
                '((http://addrs-testing.search.yandex.net/search/' + name + '/yandsearch?origin=fake&ms=xml&text=cafe&lr=213&lang=ru ' + name + '))')
        else:
            name = 'tst'
            outline.append(name)
            outline.append('((https://l7test.yandex.ru/maps ' + name + '))')
            outline.append(
                '((http://addrs-testing.search.yandex.net/yandsearch?origin=fake&ms=xml&text=cafe&lr=213&lang=ru ' + name + '))')
        config = copy.deepcopy(default)
        config.update(c)
        smap = {}
        for key, cgi in config.get('cgi', {}).iteritems():
            if key == "source":
                ss, tg = cgi[0].split(":")
                smap[ss] = tg

        # Deduce middle
        back = config['backend']

        back_to_tag = {
            'default': 'fulltesting',
            'stable': 'stable',
            'logtest': 'logtest',
        }

        outline.append(back_to_tag.get(back, back))
        default_target = 'fulltesting' if back_to_tag.get(back, back) == 'fulltesting' else 'stable'

        if 'default' in smap:
            default_target = smap['default']

        if 'search' in smap:
            default_target = smap['search']

        for s in sources:
            outline.append(smap.get(s, default_target))

        res[name] = outline

    for k in sorted(res.keys()):
        outfile.write('||' + '|'.join(all_tag(res[k])) + '||')
        outfile.write('\n')

    outfile.write('|#\n')


if __name__ == "__main__":
    config = json.load(open('testing_configurations.json', 'r'))
    gen_wiki_page(config['configurations'], open('wiki.txt', 'w'))
