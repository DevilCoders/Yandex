import grpc
import pytest

import yc_as_client as ycas


def test_integration():
    grpc_channel = grpc.insecure_channel("localhost:4286")
    client = ycas.YCAccessServiceClient(channel=grpc_channel, timeout=5)

    result = client.authorize(
        permission="compute.instances.list",
        resource_path=[
            ycas.entities.Resource(id="foo37493987347327392", type="compute.instance"),
            ycas.entities.Resource(id="foo00000000000000000", type="resource-manager.folder"),
        ],
        subject=ycas.entities.UserAccountSubject(
            id="foo70000000000000002",
        ),
    )
    assert result == ycas.entities.UserAccountSubject(id="foo70000000000000002")

    result = client.authorize(
        permission="compute.instances.list",
        resource_path=ycas.entities.Resource(id="foo00000000000000000", type="resource-manager.folder"),
        subject=ycas.entities.UserAccountSubject(
            id="foo70000000000000002",
        ),
    )
    assert result == ycas.entities.UserAccountSubject(id="foo70000000000000002")

    with pytest.raises(ycas.exceptions.PermissionDeniedException):
        client.authorize(
            permission="compute.instances.list",
            resource_path=ycas.entities.Resource(id="foo00000000000000000", type="resource-manager.folder"),
            subject=ycas.entities.UserAccountSubject(
                id="foo70000000000000004",
            ),
        )

    with pytest.raises(ycas.exceptions.BadRequestException):
        client.authorize(
            permission="compute.instance.get",
            resource_path=ycas.entities.Resource(type="resource-manager.folder", id="foo00000000000000000"),
            subject=ycas.entities.UserAccountSubject(id="foo7000000000000002"),
        )


@pytest.skip("TODO: skip reason")
def test_resource_does_not_exist():
    grpc_channel = grpc.insecure_channel("localhost:4286")
    client = ycas.YCAccessServiceClient(channel=grpc_channel)

    with pytest.raises(ycas.exceptions.PermissionDeniedException):
        client.authorize(
            permission="compute.instances.list",
            resource_path=ycas.entities.Resource(id="foo0zzzzzzzzzzzzzzzz", type="resource-manager.folder"),
            subject=ycas.entities.UserAccountSubject(
                id="foo70000000000000002",
            ),
        )

    with pytest.raises(ycas.exceptions.PermissionDeniedException):
        client.authorize(
            permission="compute.instances.list",
            resource_path=ycas.entities.Resource(id="foo00000000000000000", type="spam.ham"),
            subject=ycas.entities.UserAccountSubject(
                id="foo70000000000000002",
            ),
        )
