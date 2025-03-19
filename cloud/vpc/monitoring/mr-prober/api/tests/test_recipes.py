import io

from starlette import status

from api import models
from api.client import ModelBasedHttpClient

YAML_CONF = """
#cloud-config
datasource:
  Ec2:
    strict_id: false

runcmd:
  # Set the hostname
  - hostname -b ${hostname}
  # Wait secrets decryption by skm
  - bash -c "until [ -f /etc/secrets/selfdns_token ]; do sleep 1; done"
  # Use selfdns token
  - sed -i.bak -e "s/token\s*=\s*\(.*\)$/token = $(cat /etc/secrets/selfdns_token)/" /etc/yandex/selfdns-client/default.conf
  # Run selfdns-client
  - selfdns-client --terminal --debug | tee -a /var/log/selfdns.from.cloud_init.log
  # Explicitly run cauth agent
  - agent.sh

write_files:
  - path: /etc/default/mr_prober
    content: |
      AGENT_CONFIGURATIONS_S3_PREFIX=${stand_name}/
      CLUSTER_ID=${cluster_id}
"""


def create_test_recipe(client: ModelBasedHttpClient) -> models.ClusterRecipe:
    response = client.post(
        "/recipes",
        json=models.CreateClusterRecipeRequest(
            manually_created=True,
            arcadia_path="/recipes/1",
            name="meeseeks",
            description="I'm Mr. Meeseeks! Look at me!"
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeResponse
    )

    return response.recipe


def create_test_cluster(client: ModelBasedHttpClient, recipe: models.ClusterRecipe) -> models.Cluster:
    response = client.post(
        "/clusters",
        json=models.CreateClusterRequest(
            recipe_id=recipe.id,
            name="Meeseeks test",
            slug="meeseeks-test",
            manually_created=True,
            variables=[]
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterResponse,
    )

    return response.cluster


def test_create_recipe(client: ModelBasedHttpClient):
    # 1. Create recipe
    recipe = create_test_recipe(client)

    assert recipe.name == "meeseeks"
    assert recipe.description == "I'm Mr. Meeseeks! Look at me!"
    assert recipe.manually_created
    assert recipe.arcadia_path == "/recipes/1"

    # 2. Check that recipe is available on /recipes/1
    response = client.get(f"/recipes/{recipe.id}", response_model=models.ClusterRecipeResponse)
    assert response.recipe == recipe


def test_copy_recipe(client: ModelBasedHttpClient):
    # 1. Create recipe
    recipe = create_test_recipe(client)

    # 2. Create copy of recipe
    response = client.post(
        "/recipes/copy",
        json=models.CopyClusterRecipeRequest(
            recipe_id=recipe.id
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeResponse,
    )
    copied_recipe = response.recipe

    # 3. Check copied recipe
    assert copied_recipe.id != recipe.id
    assert copied_recipe.name == recipe.name
    assert copied_recipe.description == recipe.description
    assert copied_recipe.manually_created == recipe.manually_created
    assert copied_recipe.arcadia_path == recipe.arcadia_path

    # 4. Check that both recipes available on /recipes
    response = client.get("/recipes", response_model=models.ClusterRecipeListResponse)
    assert len(response.recipes) == 2
    assert response.recipes[0] == recipe
    assert response.recipes[1] == copied_recipe


def test_copy_recipe_with_files(client: ModelBasedHttpClient):
    # 1. Create recipe
    recipe = create_test_recipe(client)

    # 2. Add file to the recipe
    response = client.post(
        f"/recipes/{recipe.id}/files",
        json=models.CreateClusterRecipeFileRequest(
            relative_file_path="main.tf"
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeFileResponse,
    )
    file = response.file

    # 3. Create copy of recipe
    response = client.post(
        "/recipes/copy",
        json=models.CopyClusterRecipeRequest(
            recipe_id=recipe.id
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeResponse,
    )
    copied_recipe = response.recipe

    # 4. Check that recipe has been copied with files
    assert len(copied_recipe.files) == 1
    assert copied_recipe.files[0].id != file.id

    # 5. Check that original recipe saved files
    response = client.get(f"/recipes/{recipe.id}", response_model=models.ClusterRecipeResponse)
    assert len(response.recipe.files) == 1
    assert response.recipe.files[0] == file


def test_add_recipe_file(client: ModelBasedHttpClient):
    # 1. Create recipe
    recipe = create_test_recipe(client)

    # 2. Add file to the recipe
    response = client.post(
        f"/recipes/{recipe.id}/files",
        json=models.CreateClusterRecipeFileRequest(
            relative_file_path="main.tf"
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeFileResponse,
    )

    assert len(response.recipe.files) == 1
    assert response.recipe.files[0].relative_file_path == "main.tf"

    # 3. Upload content of the file
    file_id = response.file.id

    response = client.put(
        f"/recipes/{recipe.id}/files/{file_id}/content",
        files={"content": ("filename", io.StringIO("resource {}"), "text/plain")},
        expected_status_code=status.HTTP_200_OK,
        response_model=models.UpdateClusterRecipeFileResponse
    )

    assert response.file.relative_file_path == "main.tf"

    # 4. Check that file is visible on /recipes/1
    response = client.get(
        f"/recipes/{recipe.id}",
        response_model=models.ClusterRecipeResponse,
    )

    assert len(response.recipe.files) == 1
    assert response.recipe.files[0].relative_file_path == "main.tf"

    # 5. Download recipe file
    response = client.get(
        f"/recipes/{recipe.id}/files/{file_id}/content",
    )

    assert response.content == b"resource {}"


def test_create_recipe_file_with_duplicate_filepath(client: ModelBasedHttpClient, mocked_s3):
    recipe = create_test_recipe(client)

    client.post(
        f"/recipes/{recipe.id}/files",
        json=models.CreateClusterRecipeFileRequest(
            relative_file_path="main.tf"
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeFileResponse
    )

    client.post(
        f"/recipes/{recipe.id}/files",
        json=models.CreateClusterRecipeFileRequest(
            relative_file_path="main.tf"
        ),
        expected_status_code=status.HTTP_409_CONFLICT
    )


def test_update_recipe_file_with_duplicate_filepath(client: ModelBasedHttpClient, mocked_s3):
    recipe = create_test_recipe(client)

    client.post(
        f"/recipes/{recipe.id}/files",
        json=models.CreateClusterRecipeFileRequest(
            relative_file_path="main.tf"
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeFileResponse
    )

    request = client.post(
        f"/recipes/{recipe.id}/files",
        json=models.CreateClusterRecipeFileRequest(
            relative_file_path="ext.tf"
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeFileResponse
    )

    client.put(
        f"/recipes/{recipe.id}/files/{request.file.id}",
        json=models.CreateClusterRecipeFileRequest(
            relative_file_path="main.tf"
        ),
        expected_status_code=status.HTTP_409_CONFLICT
    )


def test_upload_yaml_as_recipe_file(client: ModelBasedHttpClient):
    recipe = create_test_recipe(client)
    response = client.post(
        f"/recipes/{recipe.id}/files",
        json=models.CreateClusterRecipeFileRequest(
            relative_file_path="cloud-init.yaml"
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeFileResponse,
    )

    file_id = response.file.id
    client.put(
        f"/recipes/{recipe.id}/files/{file_id}/content",
        files={"content": ("filename", io.StringIO(YAML_CONF), "text/plain")},
        expected_status_code=status.HTTP_200_OK,
        response_model=models.UpdateClusterRecipeFileResponse
    )

    response = client.get(
        f"/recipes/{recipe.id}/files/{file_id}/content",
    )
    assert response.text == YAML_CONF


def test_delete_recipe(client: ModelBasedHttpClient, mocked_s3):
    recipe = create_test_recipe(client)
    client.delete(f"/recipes/{recipe.id}", response_model=models.SuccessResponse)

    client.get(f"/recipes/{recipe.id}", expected_status_code=status.HTTP_404_NOT_FOUND)


def test_delete_recipe_with_file(client: ModelBasedHttpClient, mocked_s3):
    # 1. Create recipe
    recipe = create_test_recipe(client)

    # 2. Add file to the recipe
    client.post(
        f"/recipes/{recipe.id}/files",
        json=models.CreateClusterRecipeFileRequest(
            relative_file_path="main.tf"
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeFileResponse,
    )

    # 3. Ensure that recipe has created with a file
    recipes = client.get(f"/recipes", response_model=models.ClusterRecipeListResponse).recipes
    assert len(recipes) == 1
    assert len(recipes[0].files) == 1

    # 4. Delete the recipe
    client.delete(f"/recipes/{recipe.id}", response_model=models.SuccessResponse)

    # 5. Check that recipe is not longer available
    recipes = client.get(f"/recipes", response_model=models.ClusterRecipeListResponse).recipes
    assert len(recipes) == 0


def test_cant_delete_recipe_with_clusters(client: ModelBasedHttpClient, mocked_s3):
    # 1. Create recipe
    recipe = create_test_recipe(client)

    # 2. Create cluster from this recipe
    create_test_cluster(client, recipe)

    # 3. Try to delete recipe — should be prohibited
    client.delete(f"/recipes/{recipe.id}", expected_status_code=status.HTTP_412_PRECONDITION_FAILED)


def test_create_recipe_file_is_prohibited_if_recipe_has_clusters(client: ModelBasedHttpClient, mocked_s3):
    # 1. Create recipe and cluster from it
    recipe = create_test_recipe(client)
    create_test_cluster(client, recipe)

    # 2. Try to add file to recipe — should be prohibited
    client.post(
        f"/recipes/{recipe.id}/files",
        json=models.CreateClusterRecipeFileRequest(
            relative_file_path="main.tf"
        ),
        expected_status_code=status.HTTP_412_PRECONDITION_FAILED,
    )

    # 3. Add {..., "force": true} to request allows to create file
    client.post(
        f"/recipes/{recipe.id}/files",
        json=models.CreateClusterRecipeFileRequest(
            relative_file_path="main.tf",
            force=True,
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeFileResponse,
    )


def test_delete_recipe_file_is_prohibited_if_recipe_has_clusters(client: ModelBasedHttpClient, mocked_s3):
    # 1. Create recipe
    recipe = create_test_recipe(client)

    # 2. Create file for this recipe
    response = client.post(
        f"/recipes/{recipe.id}/files",
        json=models.CreateClusterRecipeFileRequest(
            relative_file_path="main.tf",
        ),
        expected_status_code=status.HTTP_201_CREATED,
        response_model=models.CreateClusterRecipeFileResponse,
    )

    file = response.file

    # 3. Create cluster from this recipe
    create_test_cluster(client, recipe)

    # 4. Try to delete file — should fail
    client.delete(
        f"/recipes/{recipe.id}/files/{file.id}",
        expected_status_code=status.HTTP_412_PRECONDITION_FAILED,
    )

    # 5. Try to delete file with ?force=true
    client.delete(
        f"/recipes/{recipe.id}/files/{file.id}?force=true",
        expected_status_code=status.HTTP_200_OK,
    )
