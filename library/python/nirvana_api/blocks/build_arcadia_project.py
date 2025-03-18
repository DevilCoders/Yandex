from .base_block import BaseBlock
from .processor_parameters import ProcessorType


class BuildType(object):
    Release = 'release'
    Debug = 'debug'
    Profile = 'profile'
    Coverage = 'coverage'
    ReleaseWithDebugInfo = 'relwithdebinfo'
    ValgrindDebug = 'valgrind'
    ValgrindRelease = 'valgrind-release'


class PackageType(object):
    tarball = 'tarball'
    debian = 'debian'


class BuildArcadiaProject(BaseBlock):
    guid = '3cd823d9-7210-4c72-91d2-b3c3a784875a'
    name = 'Build Arcadia Project'
    parameters = [
        'arcadia_url',
        'arcadia_revision',
        'build_type',
        'targets',
        'arts',
        'definition_flags',
        'sandbox_oauth_token',
        'arcadia_patch',
        'sandbox_requirements_disk',
        'sandbox_requirements_ram',
        'checkout',
        'clear_build',
        'strip_binaries',
        'lto',
        'musl',
        'use_system_python',
        'target_platform_flags',
        'javac_options',
        'owner',
    ]
    output_names = ['ARCADIA_PROJECT']
    processor_type = ProcessorType.Sandbox


class YaPackage(BaseBlock):
    guid = '7e063067-afc7-4dcc-b09f-47087dfd0427'
    name = 'Make Ya Package'
    parameters = [
        'packages',
        'package_type',
        'use_new_format',
        'strip_binaries',
        'resource_type',
        'arcadia_patch',
        'run_tests',
        'checkout',
        'use_ya_dev',
        'sandbox_oauth_token',
        'publish_package',
        'arcadia_url',
        'arcadia_revision',
        'checkout_arcadia_from_url',
        'sandbox_requirements_disk',
        'sandbox_requirements_ram',
        'target_platform',
        'cache'
    ]
    output_names = ['YA_PACKAGE']
    processor_type = ProcessorType.Sandbox
