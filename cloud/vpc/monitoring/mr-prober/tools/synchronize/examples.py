from api.models import BashProberRunner, RegularClusterDeployPolicy
from database.models import UploadProberLogPolicy
from tools.synchronize.iac import models as iac_models
from tools.synchronize.models import FileCollection

config = iac_models.Config(
    environments={
        "prod": iac_models.Environment(
            endpoint="https://api.prober.cloud.yandex.net",
            clusters=["clusters/prod/*/cluster.yaml"],
            probers=["probers/**/prober.yaml"],
        ),
        "preprod": iac_models.Environment(
            endpoint="https://api.prober.cloud-preprod.yandex.net",
            clusters=["clusters/preprod/*/cluster.yaml"],
            probers=["probers/**/prober.yaml"],
        ),
    }
)

recipe = iac_models.Recipe(
    name="Example",
    description="Example of recipe",
    files=[
        FileCollection(directory="files/")
    ],
)

cluster = iac_models.Cluster(
    name="Example",
    slug="example",
    recipe="recipes/example/recipe.yaml",
    variables={
        "key": "value",
    },
    deploy_policy=RegularClusterDeployPolicy(
        parallelism=20,
        sleep_interval=180,
        plan_timeout=360,
        apply_timeout=3600,
    ),
)

prober = iac_models.Prober(
    name="Example",
    slug="example",
    description="Example of prober",
    runner=BashProberRunner(command="./example ${VAR_option} ${VAR_host}"),
    files=[
        FileCollection(directory="files/")
    ],
    configs=[
        iac_models.ProberConfig(
            is_prober_enabled=True,
            interval_seconds=60,
            timeout_seconds=10,
            s3_logs_policy=UploadProberLogPolicy.FAIL,
            clusters=["clusters/*/example/cluster.yaml"],
            matrix={
                "host": ["https://yandex.ru", "https://google.com"]
            },
            variables={
                "host": "${matrix.host}",
                "option": "--verbose",
            }
        )
    ]
)
