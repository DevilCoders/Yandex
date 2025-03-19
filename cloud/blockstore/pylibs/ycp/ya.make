PY3_LIBRARY()

OWNER(g:cloud-nbs)

PY_SRCS(
    __init__.py
    ycp.py
    ycp_wrapper.py
)

RESOURCE(
    templates/attach-disk.yaml                           attach-disk.yaml
    templates/attach-fs.yaml                             attach-fs.yaml
    templates/create-disk.yaml                           create-disk.yaml
    templates/create-disk-placement-group.yaml           create-disk-placement-group.yaml
    templates/create-fs.yaml                             create-fs.yaml
    templates/create-iam-token.yaml                      create-iam-token.yaml
    templates/create-instance.yaml                       create-instance.yaml
    templates/create-placement-group.yaml                create-placement-group.yaml
    templates/delete-disk.yaml                           delete-disk.yaml
    templates/delete-fs.yaml                             delete-fs.yaml
    templates/delete-image.yaml                          delete-image.yaml
    templates/delete-instance.yaml                       delete-instance.yaml
    templates/delete-snapshot.yaml                       delete-snapshot.yaml
    templates/detach-disk.yaml                           detach-disk.yaml
    templates/detach-fs.yaml                             detach-fs.yaml
    fake-responses/fake-disk.json                        fake-disk.json
    fake-responses/fake-disk-list.json                   fake-disk-list.json
    fake-responses/fake-disk-placement-group.json        fake-disk-placement-group.json
    fake-responses/fake-disk-placement-group-list.json   fake-disk-placement-group-list.json
    fake-responses/fake-filesystem.json                  fake-filesystem.json
    fake-responses/fake-filesystem-list.json             fake-filesystem-list.json
    fake-responses/fake-image-list.json                  fake-image-list.json
    fake-responses/fake-iam-token.json                   fake-iam-token.json
    fake-responses/fake-instance.json                    fake-instance.json
    fake-responses/fake-instance-list.json               fake-instance-list.json
    fake-responses/fake-placement-group.json             fake-placement-group.json
    fake-responses/fake-placement-group-list.json        fake-placement-group-list.json
    fake-responses/fake-subnet-list.json                 fake-subnet-list.json
)

PEERDIR(
    contrib/python/dateutil
    contrib/python/Jinja2

    cloud/blockstore/pylibs/clusters
    cloud/blockstore/pylibs/common

    library/python/resource
)

END()
