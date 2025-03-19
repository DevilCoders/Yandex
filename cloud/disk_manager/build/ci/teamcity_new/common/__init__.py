import os


def use_aapi_fuse():
    return True


def arcadia_branch():
    return os.getenv('arcadia_branch', 'trunk')


def arcadia_url():
    return os.getenv('arcadia_url', 'arcadia:/arc/%s/arcadia' % arcadia_branch())


def sandbox_oauth():
    return os.getenv('sandbox_oauth')


def sandbox_task_owner():
    return os.getenv('sandbox_task_owner')


def test_threads():
    return os.getenv('test_threads', 10)


def ya_make_build_system():
    return os.getenv('ya_make_build_system', 'ya')


def ya_make_build_type():
    return os.getenv('ya_make_build_type', 'release')


def ya_make_definition_flags():
    return os.getenv('ya_make_definition_flags', '-DUSE_EAT_MY_DATA -DDEBUGINFO_LINES_ONLY')


def ya_make_keep_on():
    return os.getenv('ya_make_keep_on', True)


def kill_timeout():
    return os.getenv('kill_timeout', 10800)


def ya_timeout():
    return os.getenv('ya_timeout', 10800)


def ya_make_targets():
    return os.getenv('ya_make_targets')


def ya_make_test_tag():
    return os.getenv('ya_make_test_tag')


def ya_make_test_params():
    return os.getenv('ya_make_test_params')


def ya_make_junit_report():
    return True


def ya_package_packages():
    return os.getenv('ya_package_packages')


def ya_package_clear_build():
    return os.getenv('ya_package_clear_build', True)


def ya_package_debian_distribution():
    return os.getenv('ya_package_debian_distribution', 'unstable')


def ya_package_publish_to():
    return os.getenv('ya_package_publish_to', 'yandex-cloud')


def ya_package_checkout_mode():
    return os.getenv('ya_package_checkout_mode', 'auto')


def z2_token():
    return os.getenv('z2_token')


def z2_group_to_edit():
    return os.getenv('z2_group_to_edit')


def z2_group_to_update():
    return os.getenv('z2_group_to_update')
