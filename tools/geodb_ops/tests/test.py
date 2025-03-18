import simplejson as json

import yatest.common

_GEODB_BUILDER_PATH = yatest.common.binary_path("tools/geodb_ops/geodb_ops")
_GEOBASE_DUMP_PATH = yatest.common.work_path('geobase_dump.json')
_GEODB_ALL = yatest.common.work_path('geodb.bin')


def _is_json(data):
    try:
        json.loads(data)
    except ValueError:
        return False
    else:
        return True


def _geodb_print(geodb, geodb_json, tail=[]):
    cmd = ([_GEODB_BUILDER_PATH, 'print', '--input', geodb, '--output', geodb_json] + tail)
    yatest.common.execute(cmd)


def _geodb_build(geodb, tail=[]):
    cmd = ([_GEODB_BUILDER_PATH, 'build', 'file', '--verbose', '--input', _GEOBASE_DUMP_PATH,
            '--output', geodb] + tail)
    yatest.common.execute(cmd)


def test_build_all():
    geodb = yatest.common.test_output_path('geodb.bin')
    cmd = [_GEODB_BUILDER_PATH, 'build', 'file', '--verbose', '--input', _GEOBASE_DUMP_PATH,
           '--output', geodb]
    yatest.common.execute(cmd)

    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(geodb, geodb_json)
    return {'bin': yatest.common.canonical_file(geodb),
            'json': yatest.common.canonical_file(geodb_json),
            }


def test_print_works():
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(_GEODB_ALL, geodb_json)
    return yatest.common.canonical_file(geodb_json)


def test_print_valid_json():
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(_GEODB_ALL, geodb_json)
    with open(geodb_json, 'r') as input_:
        for line in input_:
            assert _is_json(line)


def test_print_as_array_works():
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(_GEODB_ALL, geodb_json, ['--as-array'])
    return yatest.common.canonical_file(geodb_json)


def test_print_as_array_valid_json():
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(_GEODB_ALL, geodb_json, ['--as-array'])

    with open(geodb_json, 'r') as input_:
        data = json.load(input_)
        assert isinstance(data, list)


def test_print_as_dict_works():
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(_GEODB_ALL, geodb_json, ['--as-dict'])
    return yatest.common.canonical_file(geodb_json)


def test_print_as_dict_valid_json():
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(_GEODB_ALL, geodb_json, ['--as-dict'])

    with open(geodb_json, 'r') as input_:
        data = json.load(input_)
        isinstance(data, dict)


def test_build_without_names():
    geodb = yatest.common.test_output_path('geodb.bin')
    _geodb_build(geodb, ['--without-names'])
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(geodb, geodb_json)
    return {'json': yatest.common.canonical_file(geodb_json),
            'bin': yatest.common.canonical_file(geodb),
            }


def test_build_without_location():
    geodb = yatest.common.test_output_path('geodb.bin')
    _geodb_build(geodb, ['--without-location'])
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(geodb, geodb_json)
    return {'json': yatest.common.canonical_file(geodb_json),
            'bin': yatest.common.canonical_file(geodb),
            }


def test_build_without_span():
    geodb = yatest.common.test_output_path('geodb.bin')
    _geodb_build(geodb, ['--without-span'])
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(geodb, geodb_json)
    return {'json': yatest.common.canonical_file(geodb_json),
            'bin': yatest.common.canonical_file(geodb),
            }


def test_build_without_phone_codes():
    geodb = yatest.common.test_output_path('geodb.bin')
    _geodb_build(geodb, ['--without-phone-codes'])
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(geodb, geodb_json)
    return {'json': yatest.common.canonical_file(geodb_json),
            'bin': yatest.common.canonical_file(geodb),
            }


def test_build_without_time_zone():
    geodb = yatest.common.test_output_path('geodb.bin')
    _geodb_build(geodb, ['--without-time-zone'])
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(geodb, geodb_json)
    return {'json': yatest.common.canonical_file(geodb_json),
            'bin': yatest.common.canonical_file(geodb),
            }


def test_build_without_short_name():
    geodb = yatest.common.test_output_path('geodb.bin')
    _geodb_build(geodb, ['--without-short-name'])
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(geodb, geodb_json)
    return {'json': yatest.common.canonical_file(geodb_json),
            'bin': yatest.common.canonical_file(geodb),
            }


def test_build_without_ambiguous():
    geodb = yatest.common.test_output_path('geodb.bin')
    _geodb_build(geodb, ['--without-ambiguous'])
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(geodb, geodb_json)
    return {'json': yatest.common.canonical_file(geodb_json),
            'bin': yatest.common.canonical_file(geodb),
            }


def test_build_without_population():
    geodb = yatest.common.test_output_path('geodb.bin')
    _geodb_build(geodb, ['--without-population'])
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(geodb, geodb_json)
    return {'json': yatest.common.canonical_file(geodb_json),
            'bin': yatest.common.canonical_file(geodb),
            }


def test_build_without_services():
    geodb = yatest.common.test_output_path('geodb.bin')
    _geodb_build(geodb, ['--without-services'])
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(geodb, geodb_json)
    return {'json': yatest.common.canonical_file(geodb_json),
            'bin': yatest.common.canonical_file(geodb),
            }


def test_build_without_native_name():
    geodb = yatest.common.test_output_path('geodb.bin')
    _geodb_build(geodb, ['--without-native-name'])
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(geodb, geodb_json)
    return {'json': yatest.common.canonical_file(geodb_json),
            'bin': yatest.common.canonical_file(geodb),
            }


def test_build_without_synonym_names():
    geodb = yatest.common.test_output_path('geodb.bin')
    _geodb_build(geodb, ['--without-synonym-names'])
    geodb_json = yatest.common.test_output_path('geodb.bin.json')
    _geodb_print(geodb, geodb_json)
    return {'json': yatest.common.canonical_file(geodb_json),
            'bin': yatest.common.canonical_file(geodb),
            }
