import json
from datetime import datetime
from cloud.mdb.internal.python.compute.disks.models import DiskModel, State


sample_model = DiskModel(
    id='disk-id1',
    folder_id='folder-id1',
    name='disk-name',
    size=42,
    zone_id='us-west-1',
    type_id='type-id',
    created_at=datetime.fromisoformat('2011-11-04T00:05:23'),
    instance_ids=['instance-id1'],
    status=State.CREATING,
)

sample_model_js = """{
    "created_at": "2011-11-04T00:05:23",
    "folder_id": "folder-id1",
    "id": "disk-id1",
    "instance_ids": [
        "instance-id1"
    ],
    "name": "disk-name",
    "size": 42,
    "status": "CREATING",
    "type_id": "type-id",
    "zone_id": "us-west-1"
}"""


class TestDiskModel:
    def test_to_json(self):
        assert json.dumps(sample_model.to_json(), sort_keys=True, indent=4) == sample_model_js

    def test_from_json(self):
        assert DiskModel.from_json(json.loads(sample_model_js)) == sample_model
