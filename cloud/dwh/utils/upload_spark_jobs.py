import os
from yt import wrapper as yt

yt.config["proxy"]["url"] = "hahn"


ARC_ROOT = "/Users/soin08/arc/arcadia"
YT_ROOT = "//home/cloud_analytics/dwh"
LOCAL_SPARK_JOBS_ROOT = f"{ARC_ROOT}/cloud/dwh/spark"
YT_SPARK_JOBS_ROOT = f"{YT_ROOT}/spark"
FILES_BLACKLIST = ("__init__.py", "ya.make", )


for root, dirs, files in os.walk(LOCAL_SPARK_JOBS_ROOT):
    for file in files:
        local_path = os.path.join(root, file)
        if file in FILES_BLACKLIST or os.path.basename(os.path.dirname(local_path)) == "jobs":
            continue
        yt_path = local_path.replace(LOCAL_SPARK_JOBS_ROOT, YT_SPARK_JOBS_ROOT)
        print(f"Adding {local_path} to {yt_path}")
        with open(local_path, 'rb') as f:
            yt.create(type="file", path=yt_path, recursive=True, force=True)
            yt.write_file(yt_path, f)
        # print(os.path.dirname(local_path))
        # archive.write(path, zip_path)
