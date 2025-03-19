import json
from typing import List
from typing import Tuple

import vh
import yt.wrapper as yt
from yt.wrapper.ypath import ypath_split

from cloud.analytics.nirvana.vh.types import MRTable

Data = str
FileName = str

# /Library/util/archive
CREATE_TAR_10 = '961eda13-7e75-4e8e-96aa-fa43c1dd97c1'  # Create TAR archive 10
EXTRACT_FROM_TAR = 'cb958bd8-4222-421b-8e83-06c01897a725'  # Extract from tar

# /Library/services/yql
YQL_1 = '15a84ab6-5d99-4fbe-8cf0-8857e6d673d6'  # YQL 1
YQL_2 = 'e09f3310-ddf0-4faf-a5c1-1b32ceb5701b'  # YQL 2
YQL_4 = '8f61b2a6-93ea-4880-abec-d736383f1426'  # YQL 4
PACK_TABLE_FOR_YQL = '811fb1ae-e48b-11e6-a873-0025909427cc'  # PACK_TABLE_FOR_YQL_OID

# /Library/services/yt/tables
GET_MR_TABLE = '6ef6b6f1-30c4-4115-b98c-1ca323b50ac0'  # Get MR Table


@vh.lazy(
    vh.File,
    mr_directory=vh.mkinput(vh.File, help="JSON with cluster and path keys"),
    yt_token=vh.Secret
)
def latest_non_empty_table(mr_directory, yt_token):
    data = json.load(open(mr_directory))
    yt.config['proxy']['url'] = data['cluster']
    yt.config['token'] = yt_token.value
    paths = [p for p in yt.list(data['path'], absolute=True) if yt.get_attribute(p, "row_count") > 0]
    latest = max(paths, key=lambda p: yt.get_attribute(p, "creation_time"))
    _, filename = ypath_split(latest)
    with open(filename, 'w') as outfile:
        json.dump(
            MRTable(data['cluster'], table=latest).as_dict(),
            outfile
        )
    return filename


def create_tar_archive(files: List[Tuple[FileName, Data]], **options):
    inputs = {}

    for i, (filename, data) in enumerate(files):
        inputs['file' + str(i)] = data
        options['name' + str(i)] = filename

    create_tar = vh.op(id=CREATE_TAR_10)

    return create_tar(
        _inputs=inputs,
        _options=options
    )
