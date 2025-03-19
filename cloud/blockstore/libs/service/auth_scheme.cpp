#include "auth_scheme.h"

namespace NCloud::NBlockStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

TString ToString(EPermission permission)
{
    switch (permission) {
        case EPermission::Get:
            return "nbsInternal.disks.get";
        case EPermission::List:
            return "nbsInternal.disks.list";
        case EPermission::Create:
            return "nbsInternal.disks.create";
        case EPermission::Update:
            return "nbsInternal.disks.update";
        case EPermission::Delete:
            return "nbsInternal.disks.delete";
        case EPermission::Read:
            return "nbsInternal.disks.read";
        case EPermission::Write:
            return "nbsInternal.disks.write";
        case EPermission::MAX:
            Y_FAIL("EPermission::MAX is invalid");
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TPermissionList CreatePermissionList(
    std::initializer_list<EPermission> permissions)
{
    TPermissionList result;
    for (EPermission permission : permissions) {
        result.Set(static_cast<size_t>(permission));
    }
    return result;
}

TVector<TString> GetPermissionStrings(const TPermissionList& permissions)
{
    TVector<TString> result;
    Y_FOR_EACH_BIT(permission, permissions) {
        result.push_back(ToString(static_cast<EPermission>(permission)));
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////

TPermissionList GetRequestPermissions(EBlockStoreRequest requestType)
{
    switch (requestType) {
        case EBlockStoreRequest::CmsAction:
            return CreatePermissionList({
                EPermission::Update,
                EPermission::Write});
        // The following 4 don't deal with user data.
        case EBlockStoreRequest::Ping:
        case EBlockStoreRequest::UploadClientMetrics:
        case EBlockStoreRequest::DiscoverInstances:
        case EBlockStoreRequest::QueryAvailableStorage:
        // UnmountVolume can't expose or corrupt any user data.
        case EBlockStoreRequest::UnmountVolume:
        // The following 5 can only be sent via an active endpoint.
        case EBlockStoreRequest::ReadBlocks:
        case EBlockStoreRequest::WriteBlocks:
        case EBlockStoreRequest::ZeroBlocks:
        case EBlockStoreRequest::ReadBlocksLocal:
        case EBlockStoreRequest::WriteBlocksLocal:
            return CreatePermissionList({});

        case EBlockStoreRequest::MountVolume:
            Y_FAIL("MountVolume must have been handled separately");

        // TODO: restore EPermission::Update for AssignVolume: NBS-2309.
        case EBlockStoreRequest::AssignVolume:
            return CreatePermissionList({});
        case EBlockStoreRequest::CreateVolume:
        case EBlockStoreRequest::CreatePlacementGroup:
            return CreatePermissionList({EPermission::Create});
        case EBlockStoreRequest::DestroyVolume:
        case EBlockStoreRequest::DestroyPlacementGroup:
            return CreatePermissionList({EPermission::Delete});
        case EBlockStoreRequest::ResizeVolume:
        case EBlockStoreRequest::AlterVolume:
        case EBlockStoreRequest::AlterPlacementGroupMembership:
            return CreatePermissionList({EPermission::Update});
        case EBlockStoreRequest::StatVolume:
            return CreatePermissionList({EPermission::Get});
        case EBlockStoreRequest::CreateCheckpoint:
            return CreatePermissionList({
                EPermission::Read,
                EPermission::Write});
        case EBlockStoreRequest::DeleteCheckpoint:
            return CreatePermissionList({
                EPermission::Read,
                EPermission::Write});
        case EBlockStoreRequest::GetChangedBlocks:
            return CreatePermissionList({EPermission::Read});
        case EBlockStoreRequest::DescribeVolume:
        case EBlockStoreRequest::DescribeVolumeModel:
        case EBlockStoreRequest::DescribePlacementGroup:
        case EBlockStoreRequest::DescribeEndpoint:
            return CreatePermissionList({EPermission::Get});
        case EBlockStoreRequest::ListVolumes:
        case EBlockStoreRequest::ListPlacementGroups:
            return CreatePermissionList({EPermission::List});

        case EBlockStoreRequest::StartEndpoint:
            return CreatePermissionList({
                EPermission::Read,
                EPermission::Write});
        // TODO: restore {EPermission::Read, EPermission::Write} for *Endpoint requests: NBS-2309.
        case EBlockStoreRequest::StopEndpoint:
        case EBlockStoreRequest::KickEndpoint:
            return CreatePermissionList({});

        case EBlockStoreRequest::ListEndpoints:
        case EBlockStoreRequest::ListKeyrings:
            return CreatePermissionList({EPermission::List});

        case EBlockStoreRequest::ExecuteAction:
        case EBlockStoreRequest::DescribeDiskRegistryConfig:
        case EBlockStoreRequest::UpdateDiskRegistryConfig:
            return TPermissionList().Flip();  // Require admin permissions.

        case EBlockStoreRequest::MAX:
            Y_FAIL("EBlockStoreRequest::MAX is not valid");
    }
}

TPermissionList GetMountPermissions(
    const NProto::TMountVolumeRequest& request)
{
    const auto mode = request.GetVolumeAccessMode();
    switch (mode) {
        case NProto::VOLUME_ACCESS_READ_ONLY:
        case NProto::VOLUME_ACCESS_USER_READ_ONLY:
            return CreatePermissionList({EPermission::Read});
        case NProto::VOLUME_ACCESS_READ_WRITE:
        case NProto::VOLUME_ACCESS_REPAIR:
            return CreatePermissionList({
                EPermission::Read,
                EPermission::Write});
        default:
            // In case we get unknown volume access mode, assume
            // read-only permission.
            Y_VERIFY_DEBUG(false, "Unknown EVolumeAccessMode: %d", mode);
            return CreatePermissionList({EPermission::Read});
    }
}

TPermissionList GetRequestPermissions(
    const NProto::TMountVolumeRequest& request)
{
    return GetMountPermissions(request);
}

}   // namespace NCloud::NBlockStore
