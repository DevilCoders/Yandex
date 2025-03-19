#!/usr/bin/env python3

"""
Tool for compiling all Yandex Cloud's proto specs and generating Python stubs for them.

Shortly, it's a wrapper for
python3 -m grpc_tools -I...:... --python-out=. --grpc_python_out=. <ALL_PROTO_FILES_HERE>

For now, we include private-api's proto specs and common-api's specs.
"""

import glob
import pkg_resources

from grpc_tools import protoc

# Relative path from the Mr. Prober's root folder
arcadia_cloud_bitbucket_path = "../../../bitbucket"

protos = (
        glob.glob(f"{arcadia_cloud_bitbucket_path}/private-api/yandex/**/*.proto", recursive=True) +
        glob.glob(f"{arcadia_cloud_bitbucket_path}/common-api/yandex/**/*.proto", recursive=True)
)

# Next line is inspired by source code of grpc_tools/protoc.py
proto_include = pkg_resources.resource_filename("grpc_tools", "_proto")

protoc.main(
    (
        "",
        f"-Igoogleapis/:{arcadia_cloud_bitbucket_path}/common-api/:{arcadia_cloud_bitbucket_path}/private-api/:{proto_include}",
        "-I" + ":".join(
            [
                "googleapis/",
                f"{arcadia_cloud_bitbucket_path}/common-api/",
                f"{arcadia_cloud_bitbucket_path}/private-api/",
                proto_include,
            ]
        ),
        "--python_out=.",
        "--grpc_python_out=.",
        *protos
    )
)
