import yatest.common as yc

_TRIE_OPS_PATH = [yc.binary_path('tools/trie_ops/trie_ops'), ]
_TRIE_COMPILER_PATH = _TRIE_OPS_PATH + ['compile', ]
_PRINT_TRIE_PATH = _TRIE_OPS_PATH + ['print', ]
_TRIE_TEST_PATH = _TRIE_OPS_PATH + ['test', ]

_AZ_77_100_20000_TSV = yc.work_path('az77-100-20000.tsv')

data = lambda name: yc.test_source_path('data/' + name)
_UTF_8_NO_VALUES_TSV = data('utf8-no_values.tsv')
_UTF_8_NO_VALUES_WITH_EMPTY_KEY_TSV = data('utf8-no_values-with_empty_key.tsv')
_UTF_8_UI32_TSV = data('utf8-ui32.tsv')
_UTF_8_UI32_FAILURE_TSV = data('utf8-ui32-failure.tsv')
_UTF_8_I32_TSV = data('utf8-i32.tsv')
_UTF_8_I32_FAILURE_TSV = data('utf8-i32-failure.tsv')
_UTF_8_UI64_TSV = data('utf8-ui64.tsv')
_UTF_8_UI64_FAILURE_TSV = data('utf8-ui64-failure.tsv')
_UTF_8_I64_TSV = data('utf8-i64.tsv')
_UTF_8_I64_FAILURE_TSV = data('utf8-i64-failure.tsv')
_UTF_8_FLOAT_TSV = data('utf8-float.tsv')
_UTF_8_FLOAT_FAILURE_TSV = data('utf8-float-failure.tsv')
_UTF_8_DOUBLE_TSV = data('utf8-double.tsv')
_UTF_8_DOUBLE_FAILURE_TSV = data('utf8-double-failure.tsv')
_UTF_8_WTROKA_TSV = data('utf8-wtroka.tsv')
_UTF_8_WTROKA_WITH_EMPTY_VALUE_TSV = data('utf8-wtroka-with_empty_value.tsv')
_UTF_8_WTROKA_WITH_EMPTY_KEY_AND_VALUE_TSV = data('utf8-wtroka-with_empty_key_and_value.tsv')
_UTF_8_UI32_WITH_EMPTY_KEY_TSV = data('utf8-ui32-with_empty_key.tsv')
_UTF_8_BOOL_TSV = data('utf8-bool.tsv')
_UTF_8_BOOL_FAILURE_TSV = data('utf8-bool-failure.tsv')
_UTF_8_UI32_ARRAY_TSV = data('utf8-ui32_array.tsv')
_UTF_8_UI32_ARRAY_FAILURE_TSV = data('utf8-ui32_array-failure.tsv')
_UTF_8_I32_ARRAY_TSV = data('utf8-i32_array.tsv')
_UTF_8_I32_ARRAY_FAILURE_TSV = data('utf8-i32_array-failure.tsv')
_UTF_8_UI64_ARRAY_TSV = data('utf8-ui64_array.tsv')
_UTF_8_UI64_ARRAY_FAILURE_TSV = data('utf8-ui64_array-failure.tsv')
_UTF_8_I64_ARRAY_TSV = data('utf8-i64_array.tsv')
_UTF_8_I64_ARRAY_FAILURE_TSV = data('utf8-i64_array-failure.tsv')
_UTF_8_FLOAT_ARRAY_TSV = data('utf8-float_array.tsv')
_UTF_8_FLOAT_ARRAY_FAILURE_TSV = data('utf8-float_array-failure.tsv')
_UTF_8_DOUBLE_ARRAY_TSV = data('utf8-double_array.tsv')
_UTF_8_DOUBLE_ARRAY_FAILURE_TSV = data('utf8-double_array-failure.tsv')
_UTF_8_WTROKA_ARRAY_TSV = data('utf8-wtroka_array.tsv')
_UTF_8_UI32_ARRAY_WITH_EMPTY_VALUES_TSV = data('utf8-ui32_array-with_empty_values.tsv')
_UTF_8_UI32_ARRAY_WITH_EMPTY_VALUES_AND_MISSING_SEPARATOR_TSV = data('utf8-ui32_array-with_empty_values-and_missing_separator.tsv')
_UTF_8_BOOL_ARRAY_TSV = data('utf8-bool_array.tsv')
_UTF_8_BOOL_ARRAY_FAILURE_TSV = data('utf8-bool_array-failure.tsv')


def test_help_call():
    yc.execute(_TRIE_OPS_PATH + ['--help', ])


def test_compiler_help_call():
    yc.execute(_TRIE_COMPILER_PATH + ['--help', ])


def test_printer_help_call():
    yc.execute(_PRINT_TRIE_PATH + ['--help', ])


def test_test_help_call():
    yc.execute(_TRIE_TEST_PATH + ['--help', ])


def test_wide_trie_set():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['-0', '--wide', '--verbose',
                                         '--input', _UTF_8_NO_VALUES_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', '0', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_trie_set_with_empty_key_and_flag():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['-0', '--wide', '--verbose', '--allow-empty',
                                         '--input', _UTF_8_NO_VALUES_WITH_EMPTY_KEY_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', '0', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_trie_set_with_empty_key_and_without_flag():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['-0', '--wide', '--verbose',
                                         '--input', _UTF_8_NO_VALUES_WITH_EMPTY_KEY_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', '0', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_ui32():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--wide', '--verbose',
                                         '--input', _UTF_8_UI32_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui32', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path)
            )


def test_wide_ui32_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--verbose', '--wide',
                                 '--input', _UTF_8_UI32_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_i32():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'i32', '--wide', '--verbose',
                                         '--input', _UTF_8_I32_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'i32', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_asispacker_i32():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'i32', '--as-is-packer', '--verbose',
                                         '--input', _UTF_8_I32_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'i32', '--as-is-packer', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_asispacker_ui32():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--as-is-packer', '--verbose',
                                         '--input', _UTF_8_UI32_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui32', '--as-is-packer', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_i32_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('trie.tsv')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'i32', '--wide', '--verbose',
                                 '--input', _UTF_8_I32_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_ui64():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui64', '--wide', '--verbose',
                                         '--input', _UTF_8_UI64_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui64', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path)
            )


def test_wide_ui64_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'ui64', '--verbose', '--wide',
                                 '--input', _UTF_8_UI64_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_i64():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'i64', '--wide', '--verbose',
                                         '--input', _UTF_8_I64_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'i64', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_i64_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('trie.tsv')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'i64', '--wide', '--verbose',
                                 '--input', _UTF_8_I64_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_float():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'float', '--wide', '--verbose',
                                         '--input', _UTF_8_FLOAT_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'float', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_float_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('trie.tsv')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'float', '--wide', '--verbose',
                                 '--input', _UTF_8_FLOAT_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_double():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'double', '--wide', '--verbose',
                                         '--input', _UTF_8_DOUBLE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'double', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_double_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('trie.tsv')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'double', '--wide', '--verbose',
                                 '--input', _UTF_8_DOUBLE_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_TUtf16String():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'TUtf16String', '--wide', '--verbose',
                                         '--input', _UTF_8_WTROKA_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'TUtf16String', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_wtroka():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('trie.tsv')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'wtroka', '--wide', '--verbose',
                                 '--input', _UTF_8_WTROKA_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_unknown_type():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('trie.tsv')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'unknown_type', '--wide', '--verbose',
                                 '--input', _UTF_8_WTROKA_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_big_file():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--wide', '--verbose',
                                         '--input', _AZ_77_100_20000_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui32', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_default_type():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--wide', '--verbose',
                                         '--input', _UTF_8_UI64_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui64', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path)
            )


def test_wide_default_type_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd = _TRIE_COMPILER_PATH + ['--verbose', '--wide',
                                 '--input', _UTF_8_UI64_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_minimize():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--wide', '--verbose', '--minimize',
                                         '--input', _AZ_77_100_20000_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui32', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_fast_layout():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--wide', '--verbose', '--fast-layout',
                                         '--input', _AZ_77_100_20000_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui32', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_allow_empty():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--wide', '--allow-empty',
                                         '--input', _UTF_8_UI32_WITH_EMPTY_KEY_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui32', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_allow_empty_without_flag():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--wide',
                                         '--input', _UTF_8_UI32_WITH_EMPTY_KEY_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui32', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_check():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--wide', '--verbose', '--fast-layout',
                                         '--input', _AZ_77_100_20000_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui32', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_prefix_grouped():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--wide', '--verbose', '--prefix-grouped',
                                         '--input', _AZ_77_100_20000_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui32', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_no_wide():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--verbose',
                                         '--input', _AZ_77_100_20000_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui32', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_too_many_free_arguments():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--verbose', '--wide',
                                 '--input', _UTF_8_UI32_FAILURE_TSV, trie_path, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_wtroka_with_empty_value():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'TUtf16String', '--wide', '--verbose',
                                         '--input', _UTF_8_WTROKA_WITH_EMPTY_VALUE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'TUtf16String', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_wtroka_with_empty_key_and_value():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'TUtf16String', '--wide', '--verbose', '--allow-empty',
                                         '--input', _UTF_8_WTROKA_WITH_EMPTY_KEY_AND_VALUE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'TUtf16String', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_bool():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'bool', '--wide', '--verbose',
                                         '--input', _UTF_8_BOOL_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'bool', '--wide', '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path)
            )


def test_wide_bool_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'bool', '--verbose', '--wide',
                                 '--input', _UTF_8_BOOL_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_ui32_array():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--wide', '--verbose', '--array',
                                         '--input', _UTF_8_UI32_ARRAY_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui32', '--wide', '--array',
                                    '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path)
            )


def test_wide_ui32_array_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--verbose', '--wide', '--array',
                                 '--input', _UTF_8_UI32_ARRAY_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_i32_array():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'i32', '--wide', '--verbose', '--array',
                                         '--input', _UTF_8_I32_ARRAY_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'i32', '--wide', '--array',
                                    '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_i32_array_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('trie.tsv')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'i32', '--wide', '--verbose', '--array'
                                 '--input', _UTF_8_I32_ARRAY_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_ui64_array():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui64', '--wide', '--verbose', '--array',
                                         '--input', _UTF_8_UI64_ARRAY_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui64', '--wide', '--array',
                                    '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path)
            )


def test_wide_ui64_array_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'ui64', '--verbose', '--wide', '--array',
                                 '--input', _UTF_8_UI64_ARRAY_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_i64_array():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'i64', '--wide', '--verbose', '--array',
                                         '--input', _UTF_8_I64_ARRAY_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'i64', '--wide', '--array',
                                    '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_i64_array_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('trie.tsv')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'i64', '--wide', '--verbose', '--array',
                                 '--input', _UTF_8_I64_ARRAY_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_float_array():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'float', '--wide', '--verbose', '--array',
                                         '--input', _UTF_8_FLOAT_ARRAY_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'float', '--wide', '--array',
                                    '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_float_array_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('trie.tsv')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'float', '--wide', '--verbose', '--array',
                                 '--input', _UTF_8_FLOAT_ARRAY_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_double_array():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'double', '--wide', '--verbose', '--array',
                                         '--input', _UTF_8_DOUBLE_ARRAY_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'double', '--wide', '--array',
                                    '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_double_array_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('trie.tsv')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'double', '--wide', '--verbose', '--array',
                                 '--input', _UTF_8_DOUBLE_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code


def test_wide_TUtf16String_array():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'TUtf16String', '--wide', '--verbose', '--array',
                                         '--input', _UTF_8_WTROKA_ARRAY_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'TUtf16String', '--wide', '--array',
                                    '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path),
            )


def test_wide_ui32_array_with_empty_values():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--wide', '--verbose', '--array',
                                         '--input', _UTF_8_UI32_ARRAY_WITH_EMPTY_VALUES_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui32', '--wide', '--array',
                                    '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path)
            )


def test_wide_ui32_array_with_empty_values_and_empty_separator():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'ui32', '--wide', '--verbose', '--array',
                                         '--input', _UTF_8_UI32_ARRAY_WITH_EMPTY_VALUES_AND_MISSING_SEPARATOR_TSV,
                                         trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'ui32', '--wide', '--array',
                                    '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path)
            )


def test_wide_bool_array():
    trie_path = yc.test_output_path('trie.trie')
    tsv_path = yc.test_output_path('trie.tsv')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd_compile = _TRIE_COMPILER_PATH + ['--type', 'bool', '--wide', '--verbose', '--array',
                                         '--input', _UTF_8_BOOL_ARRAY_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        yc.execute(cmd_compile, stderr=stderr)

    cmd_print = _PRINT_TRIE_PATH + ['--type', 'bool', '--wide', '--array',
                                    '--output', tsv_path, trie_path, ]
    yc.execute(cmd_print)

    return (yc.canonical_file(trie_path),
            yc.canonical_file(tsv_path)
            )


def test_wide_bool_array_failure():
    trie_path = yc.test_output_path('trie.trie')
    stderr_path = yc.test_output_path('stderr.txt')

    cmd = _TRIE_COMPILER_PATH + ['--type', 'bool', '--verbose', '--wide', '--array',
                                 '--input', _UTF_8_BOOL_FAILURE_TSV, trie_path, ]
    with open(stderr_path, 'w') as stderr:
        eo = yc.execute(cmd, stderr=stderr, check_exit_code=False)
        assert 0 != eo.exit_code
