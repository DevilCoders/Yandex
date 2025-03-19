import yatest.common
import imp
import StringIO
import md5


def test_generate():
    build_script = yatest.common.source_path("kernel/lemmer/alpha/build_abc_data.py")
    build_module = imp.load_source('build_abcdata', build_script)
    generated_source = StringIO.StringIO()
    build_module.generate(generated_source)

    generated = yatest.common.source_path("kernel/lemmer/alpha/generated/abc_data.cpp")
    real_source = open(generated).read()

    assert md5.md5(generated_source.getvalue()).hexdigest() == md5.md5(real_source).hexdigest(), 'generated file should be equal to build_abc_data script result'
