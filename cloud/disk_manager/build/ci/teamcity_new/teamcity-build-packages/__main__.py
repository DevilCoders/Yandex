from cloud.disk_manager.build.ci.teamcity_new import common
from cloud.disk_manager.build.ci.sandbox import SandboxClient


def run():
    sandbox_client = SandboxClient(
        oauth=common.sandbox_oauth()
    )

    task_params = {
        'type': 'YA_PACKAGE',
        'owner': common.sandbox_task_owner(),
        'priority': ('SERVICE', 'NORMAL'),
        'requirements': {
            'ram': 10 * 2 ** 30
        },
        'kill_timeout': common.kill_timeout(),
    }

    ya_make_params = {
        'checkout_arcadia_from_url': common.arcadia_url(),
        'use_aapi_fuse': common.use_aapi_fuse(),
        'build_type': common.ya_make_build_type(),
        'clear_build': common.ya_package_clear_build(),
        'debian_distribution': common.ya_package_debian_distribution(),
        'publish_to': common.ya_package_publish_to(),
        'packages': common.ya_package_packages(),
        'checkout_mode': common.ya_package_checkout_mode(),
    }

    sandbox_client.run_task(
        task_params,
        **ya_make_params
    )


if __name__ == '__main__':
    run()
