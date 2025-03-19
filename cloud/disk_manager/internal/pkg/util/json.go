package util

import (
	"encoding/json"
	"time"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes/empty"
	grpc_codes "google.golang.org/grpc/codes"

	dataplane_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/dataplane/protos"
	disk_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/disks/protos"
	filesystem_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/filesystem/protos"
	images_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/images/protos"
	placementgroup_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/placementgroup/protos"
	pools_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/pools/protos"
	snapshot_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/snapshots/protos"
	transfer_protos "a.yandex-team.ru/cloud/disk_manager/internal/pkg/services/transfer/protos"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/storage"
)

////////////////////////////////////////////////////////////////////////////////

var protoByTaskType = map[string]func() proto.Message{
	"dataplane.CollectSnapshots":                    func() proto.Message { return &dataplane_protos.CollectSnapshotsTaskState{} },
	"dataplane.CreateSnapshotFromDisk":              func() proto.Message { return &dataplane_protos.CreateSnapshotFromDiskTaskState{} },
	"dataplane.CreateSnapshotFromSnapshot":          func() proto.Message { return &dataplane_protos.CreateSnapshotFromSnapshotTaskState{} },
	"dataplane.CreateSnapshotFromLegacySnapshot":    func() proto.Message { return &dataplane_protos.CreateSnapshotFromLegacySnapshotTaskState{} },
	"dataplane.TransferFromDiskToDisk":              func() proto.Message { return &dataplane_protos.TransferFromDiskToDiskTaskState{} },
	"dataplane.TransferFromSnapshotToDisk":          func() proto.Message { return &dataplane_protos.TransferFromSnapshotToDiskTaskState{} },
	"dataplane.TransferFromLegacySnapshotToDisk":    func() proto.Message { return &dataplane_protos.TransferFromSnapshotToDiskTaskState{} },
	"dataplane.DeleteSnapshot":                      func() proto.Message { return &dataplane_protos.DeleteSnapshotTaskState{} },
	"dataplane.DeleteSnapshotData":                  func() proto.Message { return &dataplane_protos.DeleteSnapshotDataTaskState{} },
	"disks.CreateEmptyDisk":                         func() proto.Message { return &disk_protos.CreateEmptyDiskTaskState{} },
	"disks.CreateOverlayDisk":                       func() proto.Message { return &disk_protos.CreateOverlayDiskTaskState{} },
	"disks.CreateDiskFromImage":                     func() proto.Message { return &disk_protos.CreateDiskFromImageTaskState{} },
	"disks.CreateDiskFromSnapshot":                  func() proto.Message { return &disk_protos.CreateDiskFromSnapshotTaskState{} },
	"disks.DeleteDisk":                              func() proto.Message { return &disk_protos.DeleteDiskTaskState{} },
	"disks.ResizeDisk":                              func() proto.Message { return &disk_protos.ResizeDiskTaskState{} },
	"disks.AlterDisk":                               func() proto.Message { return &disk_protos.AlterDiskTaskState{} },
	"disks.AssignDisk":                              func() proto.Message { return &disk_protos.AssignDiskTaskState{} },
	"disks.UnassignDisk":                            func() proto.Message { return &disk_protos.UnassignDiskTaskState{} },
	"filesystem.CreateFilesystem":                   func() proto.Message { return &filesystem_protos.CreateFilesystemTaskState{} },
	"filesystem.DeleteFilesystem":                   func() proto.Message { return &filesystem_protos.DeleteFilesystemTaskState{} },
	"filesystem.ResizeFilesystem":                   func() proto.Message { return &filesystem_protos.ResizeFilesystemTaskState{} },
	"images.CreateImageFromURL":                     func() proto.Message { return &images_protos.CreateImageFromURLTaskState{} },
	"images.CreateImageFromImage":                   func() proto.Message { return &images_protos.CreateImageFromImageTaskState{} },
	"images.CreateImageFromSnapshot":                func() proto.Message { return &images_protos.CreateImageFromSnapshotTaskState{} },
	"images.CreateImageFromDisk":                    func() proto.Message { return &images_protos.CreateImageFromDiskTaskState{} },
	"images.DeleteImage":                            func() proto.Message { return &images_protos.DeleteImageTaskState{} },
	"placement_group.CreatePlacementGroup":          func() proto.Message { return &placementgroup_protos.CreatePlacementGroupTaskState{} },
	"placement_group.DeletePlacementGroup":          func() proto.Message { return &placementgroup_protos.DeletePlacementGroupTaskState{} },
	"placement_group.AlterPlacementGroupMembership": func() proto.Message { return &placementgroup_protos.AlterPlacementGroupMembershipTaskState{} },
	"pools.AcquireBaseDisk":                         func() proto.Message { return &pools_protos.AcquireBaseDiskTaskState{} },
	"pools.CreateBaseDisk":                          func() proto.Message { return &pools_protos.CreateBaseDiskTaskState{} },
	"pools.ReleaseBaseDisk":                         func() proto.Message { return &pools_protos.ReleaseBaseDiskTaskState{} },
	"pools.RebaseOverlayDisk":                       func() proto.Message { return &pools_protos.RebaseOverlayDiskTaskState{} },
	"pools.ConfigurePool":                           func() proto.Message { return &pools_protos.ConfigurePoolTaskState{} },
	"pools.DeletePool":                              func() proto.Message { return &pools_protos.DeletePoolTaskState{} },
	"pools.ImageDeleting":                           func() proto.Message { return &pools_protos.ImageDeletingTaskState{} },
	"pools.OptimizeBaseDisks":                       func() proto.Message { return &pools_protos.OptimizeBaseDisksTaskState{} },
	"pools.RetireBaseDisk":                          func() proto.Message { return &pools_protos.RetireBaseDiskTaskState{} },
	"pools.RetireBaseDisks":                         func() proto.Message { return &pools_protos.RetireBaseDisksTaskState{} },
	"snapshots.CreateSnapshotFromDisk":              func() proto.Message { return &snapshot_protos.CreateSnapshotFromDiskTaskState{} },
	"snapshots.DeleteSnapshot":                      func() proto.Message { return &snapshot_protos.DeleteSnapshotTaskState{} },
	"transfer.TransferFromImageToDisk":              func() proto.Message { return &transfer_protos.TransferFromImageToDiskTaskState{} },
	"transfer.TransferFromSnapshotToDisk":           func() proto.Message { return &transfer_protos.TransferFromSnapshotToDiskTaskState{} },
}

type TaskStateJSON struct {
	ID                  string               `json:"id"`
	IdempotencyKey      string               `json:"idempotency_key"`
	AccountID           string               `json:"account_id"`
	TaskType            string               `json:"task_type"`
	Regular             bool                 `json:"regular"`
	Description         string               `json:"description"`
	StorageFolder       string               `json:"storage_folder"`
	CreatedAt           time.Time            `json:"created_at"`
	CreatedBy           string               `json:"created_by"`
	ModifiedAt          time.Time            `json:"modified_at"`
	GenerationID        uint64               `json:"generation_id"`
	Status              string               `json:"status"`
	ErrorCode           grpc_codes.Code      `json:"error_code"`
	ErrorMessage        string               `json:"error_message"`
	ErrorDetails        *errors.ErrorDetails `json:"error_details"`
	RetriableErrorCount uint64               `json:"retriable_error_count"`
	State               proto.Message        `json:"state"`
	Metadata            map[string]string    `json:"metadata"`
	Dependencies        []*TaskStateJSON     `json:"dependencies"`
	ChangedStateAt      time.Time            `json:"changed_state_at"`
	EndedAt             time.Time            `json:"ended_at"`
	LastHost            string               `json:"last_host"`
	LastRunner          string               `json:"last_runner"`
	ZoneID              string               `json:"zone_id"`
	CloudID             string               `json:"cloud_id"`
	FolderID            string               `json:"folder_id"`
}

func (s *TaskStateJSON) Marshal() ([]byte, error) {
	return json.Marshal(s)
}

////////////////////////////////////////////////////////////////////////////////

func TaskStateToJSON(state *storage.TaskState) *TaskStateJSON {
	var protoMessage proto.Message

	creator, ok := protoByTaskType[state.TaskType]
	if ok {
		protoMessage = creator()
		err := proto.Unmarshal(state.State, protoMessage)
		if err != nil {
			protoMessage = &empty.Empty{}
		}
	} else {
		protoMessage = &empty.Empty{}
	}

	dependencies := make([]*TaskStateJSON, 0)
	for _, dep := range state.Dependencies.List() {
		dependencies = append(dependencies, &TaskStateJSON{ID: dep})
	}

	return &TaskStateJSON{
		ID:                  state.ID,
		IdempotencyKey:      state.IdempotencyKey,
		AccountID:           state.AccountID,
		TaskType:            state.TaskType,
		Regular:             state.Regular,
		Description:         state.Description,
		StorageFolder:       state.StorageFolder,
		CreatedAt:           state.CreatedAt,
		CreatedBy:           state.CreatedBy,
		ModifiedAt:          state.ModifiedAt,
		GenerationID:        state.GenerationID,
		Status:              storage.TaskStatusToString(state.Status),
		ErrorCode:           state.ErrorCode,
		ErrorMessage:        state.ErrorMessage,
		ErrorDetails:        state.ErrorDetails,
		RetriableErrorCount: state.RetriableErrorCount,
		State:               protoMessage,
		Metadata:            state.Metadata.Vals(),
		Dependencies:        dependencies,
		ChangedStateAt:      state.ChangedStateAt,
		EndedAt:             state.EndedAt,
		LastHost:            state.LastHost,
		LastRunner:          state.LastRunner,
		ZoneID:              state.ZoneID,
		CloudID:             state.CloudID,
		FolderID:            state.FolderID,
	}
}
