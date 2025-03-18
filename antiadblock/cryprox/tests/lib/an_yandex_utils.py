import re2


def expand_an_yandex(path, link, *args):
    snd_link = re2.sub(r'(https?://)?an\.yandex\.ru(.*)', r'\1yandex.ru{}\2'.format(path), link)
    assert(snd_link != link)
    return [(link,) + args, (snd_link,) + args]
