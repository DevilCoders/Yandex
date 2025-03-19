import glob
import os
from typing import Tuple, Dict
from unittest.mock import patch

from sqlalchemy.orm import Session

import creator.main
import creator.terraform
import settings
from database.models import Cluster, ClusterRecipe, ClusterRecipeFile, ClusterVariable, ClusterDeployment, ClusterDeploymentStatus
from python_terraform import TerraformCommandError
from python_terraform.terraform import TerraformFinishedProcessResult


def create_test_recipe_and_cluster(db: Session) -> Tuple[ClusterRecipe, Cluster]:
    recipe = ClusterRecipe(id=1, name="test", description="Test", arcadia_path="recipes/test/recipe.yaml")
    db.add(recipe)
    cluster = Cluster(
        id=1, name="test", slug="test", recipe=recipe, variables=[
            ClusterVariable(name="ycp_profile", value="test"),
            ClusterVariable(name="yc_endpoint", value="https://public-api.test"),
        ]
    )
    db.add(cluster)
    db.commit()
    return recipe, cluster


def assert_cluster_deployment_has_correct_status(cluster: Cluster, db: Session, status: ClusterDeploymentStatus):
    """
    Checks that deployment exists and has correct status in the database
    """
    deployments = list(db.query(ClusterDeployment).filter(ClusterDeployment.cluster_id == cluster.id))
    assert len(deployments) == 1

    deployment: ClusterDeployment = deployments[0]
    assert deployment.status == status


def assert_creator_uploads_logs_to_s3(cluster: Cluster, mocked_s3, expected_files: Dict[str, str]):
    """
    Checks that expected files are uploaded to S3 with expected content.

    `expected_files` is a dict like {"INIT_FAILED.stdout.txt": "test stdout", ...}, so keys are suffixes of filenames,
    and values are expected file contents.
    """
    bucket = mocked_s3.Bucket(settings.MR_PROBER_LOGS_S3_BUCKET)
    files = list(bucket.objects.filter(Prefix=f"{settings.S3_PREFIX}creator/{cluster.slug}/"))
    assert len(files) == len(expected_files), f"Invalid files count have been uploaded to S3. " \
                                              f"Expected: {expected_files}, found: {[file.key for file in files]}"

    for file in files:
        file_content = file.get()["Body"].read().decode()
        for expected_file_name, expected_file_content in expected_files.items():
            if file.key.endswith(expected_file_name):
                assert file_content == expected_file_content, \
                    f"Unexpected content of the file uploaded to S3: {file.key}, " \
                    f"expected {expected_file_content!r}, found {file_content!r}"
                break
        else:
            assert False, f"Unknown file {file.key} has been uploaded to S3"


def test_recipe_files_write(db: Session):
    recipe, cluster = create_test_recipe_and_cluster(db)

    for relative_file_path in ["main.tf", "module/main.tf"]:
        file = ClusterRecipeFile(
            recipe_id=recipe.id,
            relative_file_path=relative_file_path,
            content=b"// this is an empty state file",
        )
        db.add(file)
    db.commit()

    tf = creator.terraform.TerraformEnvironment(cluster)
    dir_name = tf._tmp_directory.name
    tf_files = set(glob.glob(dir_name + "/**", recursive=True))

    assert tf_files == {
        os.path.join(dir_name, file_name) for file_name in [
            "",
            "main.tf",
            "module",
            "module/main.tf",
            "backend.tf.json",
            "provider.tf.json",
        ]
    }


def test_process_cluster_if_terraform_init_failed(db: Session, mocked_s3):
    _, cluster = create_test_recipe_and_cluster(db)

    # Mock TerraformEnvironment and make it failed on "terraform init" stage
    with patch("creator.terraform.TerraformEnvironment") as TerraformEnvironment:
        TerraformEnvironment.return_value.__enter__ = lambda self: self
        error = TerraformCommandError(10, "terraform init", out="test output", err="test stderr")
        with patch.object(TerraformEnvironment.return_value, "init", side_effect=error):
            creator.main.process_cluster(cluster, db)

    assert_cluster_deployment_has_correct_status(cluster, db, ClusterDeploymentStatus.INIT_FAILED)

    # Two files should be uploaded to S3: stdout and stderr of `terraform init`
    assert_creator_uploads_logs_to_s3(
        cluster, mocked_s3, {
            "INIT_FAILED.stdout.txt": "test output",
            "INIT_FAILED.stderr.txt": "test stderr",
        }
    )


def test_process_cluster_if_terraform_plan_failed(db: Session, mocked_s3):
    _, cluster = create_test_recipe_and_cluster(db)

    # Mock TerraformEnvironment and make it failed on "terraform plan" stage
    with patch("creator.terraform.TerraformEnvironment") as TerraformEnvironment:
        TerraformEnvironment.return_value.__enter__ = lambda self: self
        init_result = TerraformFinishedProcessResult(0, "init success", "")
        plan_error = TerraformCommandError(10, "terraform plan", out="test output", err="test stderr")
        with patch.object(TerraformEnvironment.return_value, "init", return_value=init_result), \
                patch.object(TerraformEnvironment.return_value, "plan", side_effect=plan_error):
            creator.main.process_cluster(cluster, db)

    # Check that deployment exists and has correct status in the database
    assert_cluster_deployment_has_correct_status(cluster, db, ClusterDeploymentStatus.PLAN_FAILED)

    # Three files should be uploaded to S3: stdout of `terraform init`, and stdout with stderr of `terraform plan`
    assert_creator_uploads_logs_to_s3(
        cluster, mocked_s3, {
            "init.stdout.txt": "init success",
            "PLAN_FAILED.stdout.txt": "test output",
            "PLAN_FAILED.stderr.txt": "test stderr",
        }
    )


def test_process_cluster_if_terraform_apply_failed(db: Session, mocked_s3):
    _, cluster = create_test_recipe_and_cluster(db)

    # Mock TerraformEnvironment and make it failed on "terraform apply" stage
    with patch("creator.terraform.TerraformEnvironment") as TerraformEnvironment:
        TerraformEnvironment.return_value.__enter__ = lambda self: self
        init_result = TerraformFinishedProcessResult(0, "init success", "")
        plan_result = (0, "plan stdout", "plan stderr", "plan content")
        apply_error = TerraformCommandError(10, "terraform apply", out="test output", err="test stderr")
        with patch.object(TerraformEnvironment.return_value, "init", return_value=init_result), \
                patch.object(TerraformEnvironment.return_value, "plan", return_value=plan_result), \
                patch.object(TerraformEnvironment.return_value, "apply", side_effect=apply_error):
            creator.main.process_cluster(cluster, db)

    # Check that deployment exists and has correct status in the database
    assert_cluster_deployment_has_correct_status(cluster, db, ClusterDeploymentStatus.APPLY_FAILED)

    # Six files should be uploaded to S3: stdout of `terraform init`, and stdout with stderr of `terraform plan`
    assert_creator_uploads_logs_to_s3(
        cluster, mocked_s3, {
            "init.stdout.txt": "init success",
            "plan.stdout.txt": "plan stdout",
            "plan.stderr.txt": "plan stderr",
            "plan.tfplan": "plan content",
            "APPLY_FAILED.stdout.txt": "test output",
            "APPLY_FAILED.stderr.txt": "test stderr",
        }
    )


def test_process_cluster_if_terraform_apply_successed(db: Session, mocked_s3):
    _, cluster = create_test_recipe_and_cluster(db)

    with patch("creator.terraform.TerraformEnvironment") as TerraformEnvironment:
        TerraformEnvironment.return_value.__enter__ = lambda self: self
        init_result = TerraformFinishedProcessResult(0, "init success", "")
        plan_result = (0, "plan stdout", "plan stderr", "plan content")
        apply_result = TerraformFinishedProcessResult(0, "apply success", "")
        with patch.object(TerraformEnvironment.return_value, "init", return_value=init_result), \
                patch.object(TerraformEnvironment.return_value, "plan", return_value=plan_result), \
                patch.object(TerraformEnvironment.return_value, "apply", return_value=apply_result):
            creator.main.process_cluster(cluster, db)

    # Check that deployment exists and has correct status in the database
    assert_cluster_deployment_has_correct_status(cluster, db, ClusterDeploymentStatus.COMPLETED)

    # Three files should be uploaded to S3: stdout of `terraform init`, and stdout with stderr of `terraform pla`
    assert_creator_uploads_logs_to_s3(
        cluster, mocked_s3, {
            "plan.stdout.txt": "plan stdout",
            "plan.tfplan": "plan content",
            "apply.log": "apply success",
        }
    )
