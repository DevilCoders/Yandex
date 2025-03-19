from cloud.disk_manager.build.ci.teamcity_new import common
from cloud.disk_manager.build.ci.sandbox import SandboxClient


def run():
    sandbox_client = SandboxClient(
        oauth=common.sandbox_oauth()
    )

    task_params = {
        'type': 'YA_MAKE',
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
        'build_system': common.ya_make_build_system(),
        'build_type': common.ya_make_build_type(),
        'definition_flags': common.ya_make_definition_flags(),
        'keep_on': common.ya_make_keep_on(),
        'targets': common.ya_make_targets(),
        'test_tag':  common.ya_make_test_tag(),
        'test_params': common.ya_make_test_params(),
        'junit_report': common.ya_make_junit_report(),
        'ya_timeout': common.ya_timeout(),

        'test': True,
        'test_threads': common.test_threads(),
    }

    sandbox_client.run_task(
        task_params,
        **ya_make_params
    )


if __name__ == '__main__':
    run()
