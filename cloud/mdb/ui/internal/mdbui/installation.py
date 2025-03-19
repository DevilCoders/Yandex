from enum import Enum


class InstallationType(Enum):
    aws_preprod = 'aws-preprod'
    porto_test = 'porto-test'
    porto_prod = 'porto-prod'
    compute_prod = 'compute-prod'
    compute_preprod = 'compute-preprod'

    def is_compute(self) -> bool:
        return self in {InstallationType.compute_preprod, InstallationType.compute_prod}

    def is_porto(self) -> bool:
        return self in {InstallationType.porto_test, InstallationType.porto_prod}
