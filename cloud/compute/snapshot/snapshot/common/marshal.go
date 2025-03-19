package common

import (
	"encoding/json"
	"errors"
	"fmt"
	"time"

	"github.com/golang/protobuf/ptypes"
	"github.com/golang/protobuf/ptypes/wrappers"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/snapshot_rpc"
)

// MarshalCreationInfo converts creation request from common to rpc.
func MarshalCreationInfo(d *CreationInfo) *snapshot_rpc.CreateRequest {
	return &snapshot_rpc.CreateRequest{
		Id:            d.ID,
		Base:          d.Base,
		BaseProjectId: d.BaseProjectID,
		Size:          d.Size,
		Metadata:      d.Metadata,
		Organization:  d.Organization,
		ProjectId:     d.ProjectID,
		Disk:          d.Disk,
		Name:          d.Name,
		Description:   d.Description,
		ProductId:     d.ProductID,
		ImageId:       d.ImageID,
		Public:        d.Public,
	}
}

// UnmarshalCreationInfo converts creation request from rpc to common.
func UnmarshalCreationInfo(d *snapshot_rpc.CreateRequest) *CreationInfo {
	return &CreationInfo{
		CreationMetadata: CreationMetadata{
			UpdatableMetadata: UpdatableMetadata{
				Metadata:    d.Metadata,
				Description: d.Description,
			},
			ID:           d.Id,
			Organization: d.Organization,
			ProjectID:    d.ProjectId,
			ProductID:    d.ProductId,
			ImageID:      d.ImageId,
			Name:         d.Name,
			Public:       d.Public,
		},
		Base:          d.Base,
		BaseProjectID: d.BaseProjectId,
		Size:          d.Size,
		Disk:          d.Disk,
	}
}

// MarshalSnapshotInfo converts SnapshotInfo from common to rpc.
func MarshalSnapshotInfo(d *SnapshotInfo) (*snapshot_rpc.SnapshotInfo, error) {
	t, err := ptypes.TimestampProto(d.Created)
	if err != nil {
		return nil, err
	}

	changes := make([]*snapshot_rpc.Change, 0, len(d.Changes))
	for _, c := range d.Changes {
		t1, err := ptypes.TimestampProto(c.Timestamp)
		if err != nil {
			return nil, err
		}
		changes = append(changes, &snapshot_rpc.Change{
			Timestamp: t1,
			RealSize:  c.RealSize,
		})
	}
	return &snapshot_rpc.SnapshotInfo{
		Id:            d.ID,
		Base:          d.Base,
		BaseProjectId: d.BaseProjectID,
		Size:          d.Size,
		Metadata:      d.Metadata,
		Organization:  d.Organization,
		ProjectId:     d.ProjectID,
		Disk:          d.Disk,
		Created:       t,
		RealSize:      d.RealSize,
		Public:        d.Public,
		SharedWith:    d.SharedWith,
		Changes:       changes,
		Description:   d.Description,
		Name:          d.Name,
		ProductId:     d.ProductID,
		ImageId:       d.ImageID,
		State: &snapshot_rpc.State{
			Code:        d.State.Code,
			Description: d.State.Description,
		},
	}, nil
}

// UnmarshalSnapshotInfo converts SnapshotInfo from rpc to common.
func UnmarshalSnapshotInfo(d *snapshot_rpc.SnapshotInfo) (*SnapshotInfo, error) {
	var t time.Time
	if d.Created != nil {
		var err error
		t, err = ptypes.Timestamp(d.Created)
		if err != nil {
			return nil, err
		}
	}

	var state State
	if d.State != nil {
		state.Code = d.State.Code
		state.Description = d.State.Description
	}

	changes := make([]SizeChange, 0, len(d.Changes))
	for _, c := range d.Changes {
		t1, err := ptypes.Timestamp(d.Created)
		if err != nil {
			return nil, err
		}
		changes = append(changes, SizeChange{
			Timestamp: t1,
			RealSize:  c.RealSize,
		})
	}

	return &SnapshotInfo{
		CreationInfo: CreationInfo{
			CreationMetadata: CreationMetadata{
				UpdatableMetadata: UpdatableMetadata{
					Metadata:    d.Metadata,
					Description: d.Description,
				},
				ID:           d.Id,
				Organization: d.Organization,
				ProjectID:    d.ProjectId,
				ProductID:    d.ProductId,
				ImageID:      d.ImageId,
				Name:         d.Name,
				Public:       d.Public,
			},
			Base:          d.Base,
			BaseProjectID: d.BaseProjectId,
			Size:          d.Size,
			Disk:          d.Disk,
		},
		Created:    t,
		RealSize:   d.RealSize,
		SharedWith: d.SharedWith,
		Changes:    changes,
		State:      state,
	}, nil
}

// MarshalChunkInfo converts chunk info from common to rpc.
func MarshalChunkInfo(ci ChunkInfo) *snapshot_rpc.MapResponse_Chunk {
	return &snapshot_rpc.MapResponse_Chunk{
		Hashsum:   ci.Hashsum,
		Chunksize: ci.Size,
		Offset:    ci.Offset,
		Zero:      ci.Zero,
	}
}

// UnmarshalChunkInfo converts chunk info from rpc to common.
func UnmarshalChunkInfo(r *snapshot_rpc.MapResponse_Chunk) *ChunkInfo {
	return &ChunkInfo{
		Hashsum: r.Hashsum,
		Size:    r.Chunksize,
		Offset:  r.Offset,
		Zero:    r.Zero,
	}
}

// MarshalConvertRequest converts conversion request from common to rpc.
func MarshalConvertRequest(d *ConvertRequest) *snapshot_rpc.ConvertRequest {
	return &snapshot_rpc.ConvertRequest{
		Id:           d.ID,
		Metadata:     d.Metadata,
		Organization: d.Organization,
		ProjectId:    d.ProjectID,
		Description:  d.Description,
		Name:         d.Name,
		ProductId:    d.ProductID,
		ImageId:      d.ImageID,
		Format:       d.Format,
		Bucket:       d.Bucket,
		Key:          d.Key,
		Url:          d.URL,
		Public:       d.Public,
	}
}

// UnmarshalConvertRequest converts conversion request from rpc to common.
func UnmarshalConvertRequest(d *snapshot_rpc.ConvertRequest) *ConvertRequest {
	return &ConvertRequest{
		CreationMetadata: CreationMetadata{
			UpdatableMetadata: UpdatableMetadata{
				Metadata:    d.Metadata,
				Description: d.Description,
			},
			ID:           d.Id,
			Organization: d.Organization,
			ProjectID:    d.ProjectId,
			ProductID:    d.ProductId,
			ImageID:      d.ImageId,
			Name:         d.Name,
			Public:       d.Public,
		},
		Format: d.Format,
		Bucket: d.Bucket,
		Key:    d.Key,
		URL:    d.Url,
	}
}

// UnmarshalUpdateRequest converts update request from rpc to common.
func UnmarshalUpdateRequest(d *snapshot_rpc.UpdateRequest) *UpdateRequest {
	var metadata, description *string
	var public *bool
	if d.Metadata != nil {
		metadata = &d.Metadata.Value
	}
	if d.Description != nil {
		description = &d.Description.Value
	}
	if d.Public != nil {
		public = &d.Public.Value
	}
	return &UpdateRequest{
		ID:          d.Id,
		Metadata:    metadata,
		Description: description,
		Public:      public,
	}
}

// MarshalUpdateRequest converts update request from common to rpc.
func MarshalUpdateRequest(d *UpdateRequest) *snapshot_rpc.UpdateRequest {
	var metadata, description *wrappers.StringValue
	var public *wrappers.BoolValue
	if d.Metadata != nil {
		metadata = &wrappers.StringValue{Value: *d.Metadata}
	}
	if d.Description != nil {
		description = &wrappers.StringValue{Value: *d.Description}
	}
	if d.Public != nil {
		public = &wrappers.BoolValue{Value: *d.Public}
	}
	return &snapshot_rpc.UpdateRequest{
		Id:          d.ID,
		Metadata:    metadata,
		Description: description,
		Public:      public,
	}
}

// MarshalChunkMismatch converts chunk mismatch from common to rpc.
func MarshalChunkMismatch(d *ChunkMismatch) *snapshot_rpc.ChunkMismatch {
	return &snapshot_rpc.ChunkMismatch{
		Offset:  d.Offset,
		ChunkId: d.ChunkID,
		Details: d.Details,
	}
}

// MarshalVerifyReport converts verification report from common to rpc.
func MarshalVerifyReport(d *VerifyReport) *snapshot_rpc.VerifyReport {
	mismatches := make([]*snapshot_rpc.ChunkMismatch, 0, len(d.Mismatches))
	for _, mismatch := range d.Mismatches {
		mismatches = append(mismatches, MarshalChunkMismatch(mismatch))
	}

	return &snapshot_rpc.VerifyReport{
		Id:              d.ID,
		Status:          d.Status,
		Details:         d.Details,
		ChecksumMatched: d.ChecksumMatched,
		ChunkSize:       d.ChunkSize,
		TotalChunks:     d.TotalChunks,
		EmptyChunks:     d.EmptyChunks,
		ZeroChunks:      d.ZeroChunks,
		DataChunks:      d.DataChunks,
		Mismatches:      mismatches,
	}
}

func MarshalCopyRequest(d *CopyRequest) *snapshot_rpc.CopyRequest {
	return &snapshot_rpc.CopyRequest{
		OperationCloudID:   d.OperationCloudID,
		OperationProjectID: d.OperationProjectID,

		Id:              d.ID,
		TargetId:        d.TargetID,
		TargetProjectId: d.TargetProjectID,
		Name:            d.Name,
		ImageId:         d.ImageID,
		Params: &snapshot_rpc.CopyParams{
			TaskId:             d.Params.TaskID,
			HeartbeatTimeoutMs: d.Params.HeartbeatTimeoutMs,
			HeartbeatEnabled:   d.Params.HeartbeatEnabled,
		},
	}
}

// UnmarshalCopyRequest converts copy request from rpc to common.
func UnmarshalCopyRequest(d *snapshot_rpc.CopyRequest) *CopyRequest {
	if d.Params == nil {
		d.Params = &snapshot_rpc.CopyParams{}
	}

	return &CopyRequest{
		OperationCloudID:   d.OperationCloudID,
		OperationProjectID: d.TargetProjectId,

		ID:              d.Id,
		TargetID:        d.TargetId,
		TargetProjectID: d.TargetProjectId,
		Name:            d.Name,
		ImageID:         d.ImageId,
		Params: CopyParams{
			TaskID:             d.Params.TaskId,
			HeartbeatEnabled:   d.Params.HeartbeatEnabled,
			HeartbeatTimeoutMs: d.Params.HeartbeatTimeoutMs,
		},
	}
}

// FIXME: How to setup params.WorkersCount?
func MarshalMoveRequest(d *MoveRequest) (*snapshot_rpc.MoveRequest, error) {
	req := &snapshot_rpc.MoveRequest{
		OperationCloudID:   d.OperationCloudID,
		OperationProjectID: d.OperationProjectID,
		Params: &snapshot_rpc.MoveParams{
			Offset:             d.Params.Offset,
			BlockSize:          d.Params.BlockSize,
			SkipZeroes:         d.Params.SkipZeroes,
			SkipNonexistent:    d.Params.SkipNonexistent,
			TaskId:             d.Params.TaskID,
			HeartbeatEnabled:   d.Params.HeartbeatEnabled,
			HeartbeatTimeoutMs: d.Params.HeartbeatTimeoutMs,
		},
	}

	switch src := d.Src.(type) {
	case *NbsMoveSrc:
		req.Src = &snapshot_rpc.MoveRequest_NbsSrc{
			NbsSrc: &snapshot_rpc.NbsMoveSrc{
				ClusterId:         src.ClusterID,
				DiskId:            src.DiskID,
				FirstCheckpointId: src.FirstCheckpointID,
				LastCheckpointId:  src.LastCheckpointID,
			},
		}
	case *SnapshotMoveSrc:
		req.Src = &snapshot_rpc.MoveRequest_SnapshotSrc{
			SnapshotSrc: &snapshot_rpc.SnapshotMoveSrc{
				SnapshotId: src.SnapshotID,
			},
		}
	case *URLMoveSrc:
		req.Src = &snapshot_rpc.MoveRequest_UrlSrc{
			UrlSrc: &snapshot_rpc.UrlMoveSrc{
				Bucket: src.Bucket,
				Key:    src.Key,
				Url:    src.URL,
				Format: src.Format,
			},
		}
	case *NullMoveSrc:
		req.Src = &snapshot_rpc.MoveRequest_NullSrc{
			NullSrc: &snapshot_rpc.NullMoveSrc{
				Size:      src.Size,
				BlockSize: src.BlockSize,
				Random:    src.Random,
			},
		}
	default:
		return nil, errors.New("unsupported source type in move request")
	}

	switch dst := d.Dst.(type) {
	case *NbsMoveDst:
		req.Dst = &snapshot_rpc.MoveRequest_NbsDst{
			NbsDst: &snapshot_rpc.NbsMoveDst{
				ClusterId: dst.ClusterID,
				DiskId:    dst.DiskID,
				Mount:     dst.Mount,
				// MountToken: dst.MountToken, ignore
			},
		}
	case *SnapshotMoveDst:
		req.Dst = &snapshot_rpc.MoveRequest_SnapshotDst{
			SnapshotDst: &snapshot_rpc.SnapshotMoveDst{
				SnapshotId:     dst.SnapshotID,
				ProjectId:      dst.ProjectID,
				Create:         dst.Create,
				Commit:         dst.Commit,
				Fail:           dst.Fail,
				Name:           dst.Name,
				ImageId:        dst.ImageID,
				BaseSnapshotId: dst.BaseSnapshotID,
			},
		}
	case *NullMoveDst:
		req.Dst = &snapshot_rpc.MoveRequest_NullDst{
			NullDst: &snapshot_rpc.NullMoveDst{
				Size:      dst.Size,
				BlockSize: dst.BlockSize,
			},
		}
	default:
		return nil, errors.New("unsupported destination type in move request")
	}

	return req, nil
}

// UnmarshalMoveRequest converts move request from rpc to common.
func UnmarshalMoveRequest(d *snapshot_rpc.MoveRequest) *MoveRequest {
	var src MoveSrc
	var dst MoveDst
	if snapshotSrc := d.GetSnapshotSrc(); snapshotSrc != nil {
		src = &SnapshotMoveSrc{
			SnapshotID: snapshotSrc.SnapshotId,
		}
	} else if nbsSrc := d.GetNbsSrc(); nbsSrc != nil {
		src = &NbsMoveSrc{
			ClusterID:         nbsSrc.ClusterId,
			DiskID:            nbsSrc.DiskId,
			FirstCheckpointID: nbsSrc.FirstCheckpointId,
			LastCheckpointID:  nbsSrc.LastCheckpointId,
		}
	} else if nullSrc := d.GetNullSrc(); nullSrc != nil {
		src = &NullMoveSrc{
			Size:      nullSrc.Size,
			BlockSize: nullSrc.BlockSize,
			Random:    nullSrc.Random,
		}
	} else if urlSrc := d.GetUrlSrc(); urlSrc != nil {
		src = &URLMoveSrc{
			Bucket: urlSrc.Bucket,
			Key:    urlSrc.Key,
			URL:    urlSrc.Url,
			Format: urlSrc.Format,
		}
	}

	if snapshotDst := d.GetSnapshotDst(); snapshotDst != nil {
		dst = &SnapshotMoveDst{
			SnapshotID:     snapshotDst.SnapshotId,
			ProjectID:      snapshotDst.ProjectId,
			Create:         snapshotDst.Create,
			Commit:         snapshotDst.Commit,
			Fail:           snapshotDst.Fail,
			Name:           snapshotDst.Name,
			ImageID:        snapshotDst.ImageId,
			BaseSnapshotID: snapshotDst.BaseSnapshotId,
		}
	} else if nbsDst := d.GetNbsDst(); nbsDst != nil {
		dst = &NbsMoveDst{
			ClusterID: nbsDst.ClusterId,
			DiskID:    nbsDst.DiskId,
			Mount:     nbsDst.Mount,
			// MountToken: nbsDst.MountToken, // ignore
		}
	} else if nullDst := d.GetNullDst(); nullDst != nil {
		dst = &NullMoveDst{
			Size:      nullDst.Size,
			BlockSize: nullDst.BlockSize,
		}
	}

	if d.Params == nil {
		d.Params = &snapshot_rpc.MoveParams{}
	}

	return &MoveRequest{
		OperationCloudID:   d.OperationCloudID,
		OperationProjectID: d.OperationProjectID,
		Src:                src,
		Dst:                dst,
		Params: MoveParams{
			Offset:             d.Params.Offset,
			SkipZeroes:         d.Params.SkipZeroes,
			SkipNonexistent:    d.Params.SkipNonexistent,
			BlockSize:          d.Params.BlockSize,
			TaskID:             d.Params.TaskId,
			HeartbeatEnabled:   d.Params.HeartbeatEnabled,
			HeartbeatTimeoutMs: d.Params.HeartbeatTimeoutMs,
		},
	}
}

// MarshalTaskData converts task data from common to rpc.
func MarshalTaskData(d *TaskData) (res *snapshot_rpc.TaskData, err error) {
	switch r := d.Request.(type) {
	case *DeleteRequest:
		if rq, err := MarshalDeleteRequest(r); err == nil {
			res = &snapshot_rpc.TaskData{Request: &snapshot_rpc.TaskData_DeleteRequest{DeleteRequest: rq}}
		} else {
			return nil, err
		}
	case *MoveRequest:
		if rq, err := MarshalMoveRequest(r); err == nil {
			res = &snapshot_rpc.TaskData{Request: &snapshot_rpc.TaskData_MoveRequest{MoveRequest: rq}}
		} else {
			return nil, err
		}
	case *CopyRequest:
		res = &snapshot_rpc.TaskData{Request: &snapshot_rpc.TaskData_CopyRequest{CopyRequest: MarshalCopyRequest(r)}}
	default:
		return nil, errors.New("unsupported task request type in task data")
	}
	res.Status, err = MarshalTaskStatus(&d.Status)
	return
}

// MarshalTaskData converts task data from common to rpc.
func UnmarshalTaskData(d *snapshot_rpc.TaskData) (*TaskData, error) {
	res := &TaskData{}
	switch r := d.Request.(type) {
	case *snapshot_rpc.TaskData_DeleteRequest:
		res.Request = UnmarshalDeleteRequest(r.DeleteRequest)
	case *snapshot_rpc.TaskData_MoveRequest:
		res.Request = UnmarshalMoveRequest(r.MoveRequest)
	case *snapshot_rpc.TaskData_CopyRequest:
		res.Request = UnmarshalCopyRequest(r.CopyRequest)
	default:
		return nil, errors.New("unsupported task request type in task data rpc")
	}

	st, err := UnmarshalTaskStatus(d.Status)
	if err != nil {
		return nil, err
	}
	res.Status = *st
	return res, nil

}

// MarshalTaskStatus converts task status from common to rpc.
func MarshalTaskStatus(d *TaskStatus) (*snapshot_rpc.TaskStatus, error) {
	var e *snapshot_rpc.GrpcStatus
	if d.Error != nil {
		s, ok := status.FromError(d.Error)
		if !ok {
			e = &snapshot_rpc.GrpcStatus{
				Code:    int32(codes.Internal),
				Message: NewError("ErrInternal", fmt.Sprintf("Unknown error: %s", d.Error.Error()), false).JSON(),
			}
		} else {
			e = &snapshot_rpc.GrpcStatus{
				Code:    int32(s.Code()),
				Message: s.Message(),
			}
		}
	}

	res := &snapshot_rpc.TaskStatus{
		Finished: d.Finished,
		Success:  d.Success,
		Progress: float32(d.Progress),
		Offset:   d.Offset,
		Error:    e,
	}

	if createdAt, err := ptypes.TimestampProto(d.CreatedAt); err == nil {
		res.CreatedAt = createdAt
	} else {
		return nil, err
	}
	if d.FinishedAt != nil {
		if finishedAt, err := ptypes.TimestampProto(*d.FinishedAt); err == nil {
			res.FinishedAt = finishedAt
		} else {
			return nil, err
		}
	}

	return res, nil
}

func UnmarshalTaskStatus(d *snapshot_rpc.TaskStatus) (*TaskStatus, error) {
	st := &TaskStatus{
		Finished: d.Finished,
		Success:  d.Success,
		Progress: float64(d.Progress),
		Offset:   d.Offset,
	}
	if d.Error != nil {
		st.Error = status.Error(codes.Code(d.Error.Code), d.Error.Message)
	}

	if d.CreatedAt != nil {
		createdAt, err := ptypes.Timestamp(d.CreatedAt)
		if err != nil {
			return nil, err
		}
		st.CreatedAt = createdAt
	}

	if d.FinishedAt != nil {
		finishedAt, err := ptypes.Timestamp(d.FinishedAt)
		if err != nil {
			return nil, err
		}
		st.FinishedAt = &finishedAt
	}

	return st, nil
}

func MarshalTaskInfo(d *TaskInfo) (*snapshot_rpc.TaskInfo, error) {
	st, err := MarshalTaskStatus(&d.Status)
	return &snapshot_rpc.TaskInfo{
		Id:     d.TaskID,
		Status: st,
	}, err
}

func UnmarshalTaskInfo(d *snapshot_rpc.TaskInfo) (*TaskInfo, error) {
	st, err := UnmarshalTaskStatus(d.Status)
	if err != nil {
		return nil, err
	}
	return &TaskInfo{TaskID: d.Id, Status: *st}, nil
}

func MarshalTaskInfoList(l []*TaskInfo) (rp *snapshot_rpc.ListTasksResponse, err error) {
	rp = &snapshot_rpc.ListTasksResponse{Tasks: make([]*snapshot_rpc.TaskInfo, len(l))}
	for i, d := range l {
		if rp.Tasks[i], err = MarshalTaskInfo(d); err != nil {
			return
		}
	}
	return
}

func UnmarshalTaskInfoList(l *snapshot_rpc.ListTasksResponse) (res []*TaskInfo, err error) {
	res = make([]*TaskInfo, len(l.Tasks))
	for i, d := range l.Tasks {
		if res[i], err = UnmarshalTaskInfo(d); err != nil {
			return
		}
	}
	return
}

func MarshalDeleteRequest(d *DeleteRequest) (*snapshot_rpc.DeleteRequest, error) {
	return &snapshot_rpc.DeleteRequest{
		OperationCloudID:   d.OperationCloudID,
		OperationProjectID: d.OperationProjectID,

		Id:              d.ID,
		SkipStatusCheck: d.SkipStatusCheck,
		Params: &snapshot_rpc.DeleteParams{
			TaskId:             d.Params.TaskID,
			HeartbeatEnabled:   d.Params.HeartbeatEnabled,
			HeartbeatTimeoutMs: d.Params.HeartbeatTimeoutMs,
		},
	}, nil
}

// UnmarshalDeleteRequest converts delete request from rpc to common.
func UnmarshalDeleteRequest(d *snapshot_rpc.DeleteRequest) *DeleteRequest {
	if d.Params == nil {
		d.Params = &snapshot_rpc.DeleteParams{}
	}

	return &DeleteRequest{
		OperationCloudID:   d.OperationCloudID,
		OperationProjectID: d.OperationProjectID,

		ID:              d.Id,
		SkipStatusCheck: d.SkipStatusCheck,
		Params: DeleteParams{
			TaskID:             d.Params.TaskId,
			HeartbeatEnabled:   d.Params.HeartbeatEnabled,
			HeartbeatTimeoutMs: d.Params.HeartbeatTimeoutMs,
		},
	}
}

// MarshalChangedChildrenList converts changed children from common to rpc.
func MarshalChangedChildrenList(d []ChangedChild) (*snapshot_rpc.ChangedChildrenList, error) {
	var response snapshot_rpc.ChangedChildrenList
	response.ChangedChildren = make([]*snapshot_rpc.ChangedChildrenList_ChangedChild, 0, len(d))

	for _, child := range d {
		t, err := ptypes.TimestampProto(child.Timestamp)
		if err != nil {
			return nil, err
		}

		response.ChangedChildren = append(response.ChangedChildren,
			&snapshot_rpc.ChangedChildrenList_ChangedChild{
				Id:        child.ID,
				Timestamp: t,
				RealSize:  child.RealSize,
			})
	}
	return &response, nil
}

func UnmarshalSnapshotError(st *status.Status) (*SnapshotError, bool) {
	var serr SnapshotError
	if err := json.Unmarshal([]byte(st.Message()), &serr); err != nil {
		return nil, false
	}

	return &serr, true
}

// Unwrap snapshot error from grpc.Status for further analysis with xerrors and higher level
func UnwrapSnapshotError(err error) error {
	gerr, ok := status.FromError(err)
	if !ok {
		return err // not a grpc.Status
	}
	serr, ok := UnmarshalSnapshotError(gerr)
	if !ok {
		return err // not a SnapshotError
	}
	return serr
}
