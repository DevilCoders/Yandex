import enum
import json
import logging
import os
import os.path
import subprocess
import tempfile
from typing import Optional, AnyStr, Tuple
import urllib.parse

import database.models
import settings
from python_terraform import Terraform, TerraformCommandResult, IsFlagged, TerraformCommandError


class WellKnownVariables(enum.Enum):
    YCP_PROFILE = "ycp_profile"
    YC_ENDPOINT = "yc_endpoint"


class TerraformEnvironment:
    # See https://www.terraform.io/docs/commands/plan.html#detailed-exitcode
    # Exit code 0 means empty diff, exit code 2 means non-empty diff
    class TerraformPlanExitCode(enum.IntEnum):
        EMPTY_DIFF = 0
        ERROR = 1
        NONEMPTY_DIFF = 2

    def __init__(self, cluster: database.models.Cluster):
        self._cluster = cluster
        self._tmp_directory = tempfile.TemporaryDirectory(prefix="creator-")
        self._logger = logging.getLogger(cluster.slug)
        self._logger.info(f"Creating temporary terraform environment in {self._tmp_directory.name}")

        self._write_cluster_config()
        self._prepare_environment()

        self._terraform = Terraform(
            working_dir=self._tmp_directory.name,
            variables={variable.name: variable.value for variable in self._cluster.variables},
            is_env_vars_included=True
        )

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.cleanup()

    def _save_file_in_tmp_directory(self, file_name: str, content: AnyStr, description: str):
        dir_path = os.path.join(self._tmp_directory.name, os.path.dirname(file_name))
        os.makedirs(dir_path, exist_ok=True)

        mode = "wb" if type(content) is bytes else "w"
        full_file_name = os.path.join(self._tmp_directory.name, file_name)
        with open(full_file_name, mode) as opened_file:
            opened_file.write(content)
        self._logger.debug(f"{len(content)} bytes of {description} was written to {full_file_name}")

    def _write_cluster_config(self):
        for file in self._cluster.recipe.files:
            if file.content is not None:
                self._save_file_in_tmp_directory(file.relative_file_path, file.content, "recipe's file")

    def _prepare_environment(self):
        backend_configuration = self._build_backend_configuration()
        self._save_file_in_tmp_directory("backend.tf.json", json.dumps(backend_configuration), "backend configuration")

        provider_configuration = self._build_provider_configuration()
        self._save_file_in_tmp_directory(
            "provider.tf.json", json.dumps(provider_configuration),
            "provider configuration"
        )

    def init(self) -> TerraformCommandResult:
        """
        Runs 'terraform init'
        """
        self._logger.info(f"Running 'terraform init' in {self._tmp_directory.name}")
        return self._terraform.init(input=False, raise_on_error=True, capture_output=True)

    def plan(self, **kwargs) -> Optional[Tuple[int, AnyStr, AnyStr, bytes]]:
        """
        Runs 'terraform plan' and returns plan
        """
        self._logger.info(f"Running 'terraform plan' in {self._tmp_directory.name}")
        timeout = kwargs.pop("timeout", None)
        plan_file_name = os.path.join(self._tmp_directory.name, "plan.tfplan")
        plan_result = self._terraform.plan(
            input=False,
            out=plan_file_name,
            detailed_exitcode=IsFlagged,
            synchronous=False,
            **kwargs
        )

        # Copied from https://github.com/beelit94/python-terraform/blob/release/0.10.1/python_terraform/__init__.py#L295
        # to add timeout processing
        try:
            stdout, stderr = plan_result.process.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            plan_result.process.kill()
            self._logger.info(f"'terraform plan' killed by timeout {timeout} seconds.")
            stdout, stderr = plan_result.process.communicate()
            raise TerraformCommandError(plan_result.process.returncode, "terraform plan ...", out=stdout, err=stderr)

        return_code = plan_result.process.returncode

        if return_code != self.TerraformPlanExitCode.ERROR:
            self._terraform.read_state_file()
        self._terraform.temp_var_files.clean_up()

        if return_code == self.TerraformPlanExitCode.EMPTY_DIFF:
            self._logger.info(f"'terraform plan' exited with code {return_code}. It means that diff IS empty")
            return None

        if return_code == self.TerraformPlanExitCode.NONEMPTY_DIFF:
            self._logger.info(f"'terraform plan' exited with code {return_code}. It means that diff IS NOT empty")
            self._logger.info(f"Plan was temporary saved to {plan_file_name}")
            with open(plan_file_name, "rb") as plan_file:
                plan_content = plan_file.read()
            self._logger.debug(f"Plan length is {len(plan_content)} bytes")

            return_code, stdout, stderr = self._terraform.cmd(
                "show", plan_file_name, no_color=IsFlagged, raise_on_error=True
            )

            return return_code, stdout, stderr, plan_content

        if return_code == self.TerraformPlanExitCode.ERROR:
            raise TerraformCommandError(return_code, "terraform plan ...", out=stdout, err=stderr)

        raise ValueError(f"Unknown return code of 'terraform plan': {return_code}")

    def apply(self, plan: bytes, **kwargs) -> TerraformCommandResult:
        """
        Runs 'terraform apply' with specified plan
        """
        plan_file_fd, plan_file_name = tempfile.mkstemp(suffix=".tfplan", dir=self._tmp_directory.name, text=False)
        self._logger.info(f"Saving plan to {plan_file_name}")
        with open(plan_file_fd, "wb") as plan_file:
            plan_file.write(plan)

        self._logger.info(f"Running 'terraform apply' in {self._tmp_directory.name}")
        timeout = kwargs.pop("timeout", None)
        # var=None is needed due to https://github.com/beelit94/python-terraform/issues/67
        apply_result = self._terraform.apply(
            plan=plan_file_name,
            input=False, var=None,
            raise_on_error=True,
            synchronous=False,
            **kwargs
        )

        # Copied from https://github.com/beelit94/python-terraform/blob/release/0.10.1/python_terraform/__init__.py#L295
        # to add timeout processing
        try:
            stdout, stderr = apply_result.process.communicate(timeout=timeout)
            return_code = apply_result.process.returncode
        except subprocess.TimeoutExpired:
            apply_result.process.kill()
            stdout, stderr = apply_result.process.communicate()
            self._logger.warning(f"'terraform apply' killed by timeout {timeout} seconds.")
            raise TerraformCommandError(apply_result.process.returncode, "terraform apply ...", out=stdout, err=stderr)

        if return_code == 0:
            self._terraform.read_state_file()
        self._terraform.temp_var_files.clean_up()

        if return_code != 0:
            raise TerraformCommandError(return_code, "terraform apply ...", out=stdout, err=stderr)

        return return_code, stdout, stderr

    def migrate_to_yc_terraform_mirror(self):
        """
        Temporary method for migrating remote state for running terraform into Yandex Cloud's terraform mirror.
        See https://clubs.at.yandex-team.ru/ycp/4790.
        Can be removed after migrating all clusters on all stands.
        """
        self._logger.info(f"Running 'terraform state replace-providers' for further usage with terraform mirror")
        provider_changes = {
            "terraform-registry.storage.yandexcloud.net/yandex-cloud/yandex": "yandex-cloud/yandex",
            "terraform-registry.storage.yandexcloud.net/hashicorp/external": "hashicorp/external",
        }
        for old_name, new_name in provider_changes.items():
            self._terraform.cmd(
                "state replace-provider",
                old_name, new_name,
                auto_approve=IsFlagged,
                raise_on_error=True,
            )

    def cleanup(self):
        self._tmp_directory.cleanup()

    def _build_backend_configuration(self):
        s3_endpoint_for_terraform = urllib.parse.urlparse(settings.S3_ENDPOINT).netloc
        # In tests, S3_ENDPOINT is None, because moto3 doesn't work with custom endpoint,
        # but in this case urlparse returns b'' instead of '', which one is invalid for JSON.
        if settings.S3_ENDPOINT is None:
            s3_endpoint_for_terraform = ""

        return {
            "terraform": [
                {
                    "backend": [
                        {
                            "s3": [
                                {
                                    "bucket": settings.TERRAFORM_STATES_BUCKET_NAME,
                                    "endpoint": s3_endpoint_for_terraform,
                                    "key": settings.S3_PREFIX + str(self._cluster.id),
                                    "region": "us-east-1",
                                    "skip_credentials_validation": True,
                                    "skip_metadata_api_check": True,
                                }
                            ]
                        }
                    ]
                }
            ]
        }

    def _build_provider_configuration(self):
        return {
            "provider": [
                {
                    "ycp": [
                        {
                            "ycp_profile": self._cluster.get_variable_value(WellKnownVariables.YCP_PROFILE.value),
                        }
                    ]
                },
                {
                    "ytr": [
                        {
                            "conductor_token": settings.CONDUCTOR_TOKEN,
                        }
                    ]
                },
                {
                    "yandex": [
                        {
                            "endpoint": self._cluster.get_variable_value(WellKnownVariables.YC_ENDPOINT.value),
                        }
                    ]
                }
            ],
            # Following versions should be equal to versions installed by creator/files/terraform_providers.tf.json.
            # Edit these files simultaneously.
            "terraform": [
                {
                    "required_providers": [
                        {
                            "ycp": {
                                "source": "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp",
                                "version": "0.77.0",
                            },
                            "ytr": {
                                "source": "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ytr",
                                "version": "0.6.0",
                            },
                            "yandex": {
                                "source": "yandex-cloud/yandex",
                                "version": "0.72.0",
                            },
                            "external": {
                                "source": "hashicorp/external",
                                "version": "2.2.0",
                            },
                        }
                    ]
                }
            ]
        }
