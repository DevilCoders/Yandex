from dssclient.stamping import Stamp, to_rgb


def test_basic():

    stamp = Stamp(['some text', 'two'])
    stamp.box.bg = 'white'
    d = stamp.asdict()

    assert len(d['Content']) == 2
    assert 'Page' not in d
    assert d['TemplateId'] == 1
    assert d['Rect']['BackgroundColor'] == {'Blue': 255, 'Green': 255, 'Red': 255}

    stamp = Stamp('some text', page=3)
    d = stamp.asdict()

    assert len(d['Content']) == 1
    assert d['Page'] == 3
    assert 'BackgroundColor' not in d['Rect']

    stamp.serialize()


def test_to_rgb():

    rgb_dict1 = to_rgb((0, 0, 0))
    rgb_dict2 = to_rgb('black')

    assert rgb_dict1 == rgb_dict2


def test_images():
    stamp = Stamp()
    stamp.set_background_image(b'1', pos=(10, 10), scale=20)
    d = stamp.asdict()

    assert d['TemplateId'] == 2
    assert d['Icon']['Image'] == b'MQ=='
    assert d['Icon']['Scale'] == 20

    stamp = Stamp()
    stamp.set_foreground_image(b'1')
    d = stamp.asdict()

    assert d['TemplateId'] == 3
    assert d['Background']['Image'] == b'MQ=='


def test_markers():
    stamp = Stamp('!c:red;s:iu;S:12;m:10;f:arial!some text')
    content = stamp.asdict()['Content'][0]

    assert content['Margin'] == 10
    assert content['Font']['FontSize'] == 12
    assert content['Font']['FontStyle'] == 6
    assert content['Font']['FontFamily'] == 'arial'
    assert content['Font']['FontColor'] == {'Blue': 0, 'Green': 0, 'Red': 255}

    stamp.content[0].font.apply_markers({})
