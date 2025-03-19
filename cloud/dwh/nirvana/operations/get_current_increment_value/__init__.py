from cloud.dwh.lms.controllers.increment_controller import IncrementController
from cloud.dwh.lms.models.metadata import LMSDBLoadMetadataInc
from nirvana import job_context as nv
import cloud.dwh.lms.config as lms_config
import json


def main():
    job_context = nv.context()
    inputs = job_context.get_inputs()
    outputs = job_context.get_outputs()
    params = job_context.get_parameters()

    lms_config.YAV_OAUTH_TOKEN = params.get("yav-oauth-token")
    lms_config.METADATA_CONN_ID = params.get("metadata-conn-id")
    metadata_file_path = inputs.get("metadata")

    with open(metadata_file_path, "r") as f:
        metadata = json.load(f)

    md = LMSDBLoadMetadataInc(
        object_id=metadata["object_id"],
        load_type=metadata["load_type"],
        increment_type=metadata["increment_type"],
        increment_column_name=metadata["increment_column_name"],
        step_back_value=metadata["step_back_value"]
    )
    ic = IncrementController(metadata=md)
    inc_value = ic.get_increment_value()

    with open(outputs.get('increment_value'), 'w') as f:
        f.write(ic.increment_value_to_string(inc_value))


if __name__ == "__main__":
    main()
