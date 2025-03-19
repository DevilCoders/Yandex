import enum
from typing import NamedTuple, Dict, List, Optional
from ...dns import Record


class EC2DiscSpec(NamedTuple):
    type: str
    size: int


class EC2InstanceSpec(NamedTuple):
    boot: EC2DiscSpec
    data: EC2DiscSpec
    instance_type: str
    image_type: str
    name: str
    labels: Dict[str, str]
    userdata: str
    sg_id: str
    subnet_id: str
    iam_instance_profile: str


class EC2InstanceAddresses(NamedTuple):
    public_v4: Optional[str]
    v6: str
    private_v4: str

    def public_records(self) -> List[Record]:
        records = [
            Record(
                address=self.v6,
                record_type='AAAA',
            ),
        ]
        if self.public_v4:
            records.append(
                Record(
                    address=self.public_v4,
                    record_type='A',
                )
            )
        return records

    def private_records(self) -> List[Record]:
        return [
            Record(
                address=self.v6,
                record_type='AAAA',
            ),
            Record(
                address=self.private_v4,
                record_type='A',
            ),
        ]


@enum.unique
class EC2InstanceChange(enum.Enum):
    disk_size_up = 'disk_size_up'
    instance_type = 'instance_type'
