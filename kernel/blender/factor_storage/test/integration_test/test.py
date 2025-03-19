from kernel.blender.factor_storage.test.common import utils

import json
import os
import yatest
from yql.api.v1.client import YqlClient


class Params:
    TEST_PROGRAM_DIR = os.path.join('kernel', 'blender', 'factor_storage', 'test', 'integration_test')
    PROGRAM_PATH = {
        'cpp': os.path.join('cpp_program', 'cpp_program'),
        'py': os.path.join('python_program', 'py', 'py_program'),
        'py3': os.path.join('python_program', 'py3', 'py3_program'),
    }
    FACTORS = {
        'static_factors': utils.STATIC_FACTORS,
        'dynamic_factors': utils.DYNAMIC_FACTORS,
    }
    FACTORS_FILE_NAME = 'factors.json'

    @staticmethod
    def get_program_path(lang):
        return os.path.join(Params.TEST_PROGRAM_DIR, Params.PROGRAM_PATH[lang])

    @staticmethod
    def write_factors():
        with open(Params.FACTORS_FILE_NAME, 'w') as f:
            json.dump(Params.FACTORS, f)
        return Params.FACTORS_FILE_NAME

    @staticmethod
    def get_params(mode,  in_file, out_file):
        return [
            '--mode', mode,
            '--in_file', in_file,
            '--out_file', out_file,
        ]


def _compare_factors(f1, f2, original_factors):
    with open(f1, 'r') as f:
        fact1 = json.load(f)
    with open(f2, 'r') as f:
        fact2 = json.load(f)
    utils.assert_lists_equal(fact1['static_factors'], original_factors['static_factors'])
    utils.assert_lists_equal(fact2['static_factors'], original_factors['static_factors'])
    utils.assert_dynamic_features_equal(fact1['dynamic_factors'], original_factors['dynamic_factors'])
    utils.assert_dynamic_features_equal(fact2['dynamic_factors'], original_factors['dynamic_factors'])


def _run_binary(program_path, mode, in_file, out_file):
    yatest.common.execute(
        [yatest.common.binary_path(program_path)] +
        Params.get_params(mode=mode, in_file=in_file, out_file=out_file),
        check_exit_code=True
    )


def _compare_languages(language_1, language_2, original_factors, factors_file):
    _run_binary(Params.get_program_path(language_1), 'compress', factors_file, language_1 + 'compressed.txt')
    _run_binary(Params.get_program_path(language_2), 'compress', factors_file, language_2 + 'compressed.txt')
    _run_binary(Params.get_program_path(language_1), 'decompress',
                language_2 + 'compressed.txt', language_1 + 'decompressed.txt')
    _run_binary(Params.get_program_path(language_2), 'decompress',
                language_1 + 'compressed.txt', language_2 + 'decompressed.txt')
    _compare_factors(factors_file, language_1 + 'decompressed.txt', original_factors)
    _compare_factors(factors_file, language_2 + 'decompressed.txt', original_factors)


def test_py2_py3_interaction():
    factors_file = Params.write_factors()
    _compare_languages('py', 'py3', Params.FACTORS, factors_file)


def test_cpp_py2_interaction():
    factors_file = Params.write_factors()
    _compare_languages('cpp', 'py', Params.FACTORS, factors_file)


def test_cpp_py3_interaction():
    factors_file = Params.write_factors()
    _compare_languages('cpp', 'py3', Params.FACTORS, factors_file)


# ###############
# YQL udf test
# ###############
def _write_string_to_table(table_name, client, input_file, column_name):
    with open(input_file, 'r') as f:
        file_data = f.read()
    names = [column_name]
    types = ['String']
    data = [
        [file_data],
    ]
    client.write_table(table_name, data, names, types, external_tx='new')


def _run_yql(mode, in_file, out_file, client):
    input_table_name = 'tmp/input_table'
    output_table_name = 'tmp/output_table'
    input_column = 'data'

    _write_string_to_table(input_table_name, client, in_file, input_column)

    if mode == 'compress':
        request = client.query('''
            $convert_list = ($l)->{{RETURN ListMap($l, ($x)->{{RETURN CAST($x as float)}})}};
            $convert_dict = ($d)->{{
                return ToDict(ListMap(DictItems($d), ($p)->{{RETURN AsTuple($p.0, $convert_list(Yson::ConvertToDoubleList($p.1)))}}))
            }};

            INSERT INTO `{output_table}` WITH TRUNCATE
            SELECT
                BlenderFactorStorage::Compress(
                    $convert_list(Yson::ConvertToDoubleList(Yson::ParseJson({column_name})["static_factors"])),
                    $convert_dict(Yson::ConvertToDict(Yson::ParseJson({column_name})["dynamic_factors"]))
                ) as compressed
            FROM `{input_table}`
            '''.format(input_table=input_table_name, output_table=output_table_name,
                       column_name=input_column)
            , syntax_version=1)
    elif mode == 'decompress':
        request = client.query('''
            INSERT INTO `{output_table}` WITH TRUNCATE
            SELECT
                Yson::Serialize(Yson::From(AsStruct(
                    BlenderFactorStorage::Decompress({column_name}).StaticFactors as static_factors,
                    BlenderFactorStorage::Decompress({column_name}).DynamicFactors as dynamic_factors,
                    BlenderFactorStorage::Decompress({column_name}).IsError as is_error,
                ))) as decompressed
            FROM `{input_table}`
            '''.format(input_table=input_table_name, output_table=output_table_name,
                       column_name=input_column)
            , syntax_version=1)
    else:
        assert False, 'Unknown mode in run_yql: {}'.format(mode)

    request.run()
    results = request.get_results()
    assert results.is_success, [str(error) for error in results.errors]
    output_table_it = client.read_table(output_table_name)
    for row in output_table_it:
        output_data = row[0] if mode == 'compress' else json.dumps(row[0])
    with open(out_file, 'w') as f:
        f.write(output_data)


def _compare_language_to_yql(language, original_factors, factors_file, client):
    _run_binary(Params.get_program_path(language), 'compress', factors_file, language + 'compressed.txt')
    _run_yql('compress', factors_file, 'yql_compressed.txt', client)
    _run_binary(Params.get_program_path(language), 'decompress', 'yql_compressed.txt', language + 'decompressed.txt')
    _run_yql('decompress', language + 'compressed.txt', 'yql_decompressed.txt', client)
    _compare_factors(factors_file, 'yql_decompressed.txt', original_factors)
    _compare_factors(factors_file, language + 'decompressed.txt', original_factors)


def test_cpp_yql_interaction(tmpdir, yql_api, mongo, yt):
    factors_file = Params.write_factors()
    client = YqlClient(
        server='localhost',
        port=yql_api.port,
        db='plato',
        db_proxy='localhost:{}'.format(yt.yt_proxy_port)
    )
    _compare_language_to_yql('cpp', Params.FACTORS, factors_file, client)
