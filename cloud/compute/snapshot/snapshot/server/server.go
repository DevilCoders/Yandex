package server

import (
	"time"

	ptypes "github.com/golang/protobuf/ptypes"
	timestamp "github.com/golang/protobuf/ptypes/timestamp"
	"golang.org/x/net/context"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/config"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/chunker"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/snapshot_rpc"
)

const (
	defaultListLimit = 100
	maxListLimit     = 1000
)

type connections map[string]*lib.Snapshot

func convertTS(src *timestamp.Timestamp, dst **time.Time) error {
	if src == nil {
		return nil
	}

	var err error
	var t time.Time
	if t, err = ptypes.Timestamp(src); err != nil {
		return err
	}
	*dst = &t
	return nil
}

type snapshotServer struct {
	context.Context
	f     *lib.Facade
	conns connections
}

func newSnapshotServer(ctx context.Context, conf *config.Config) (*snapshotServer, error) {
	f, err := lib.NewFacade(ctx, conf)
	if err != nil {
		return nil, convertError(err)
	}

	return &snapshotServer{Context: ctx, f: f, conns: make(connections)}, nil
}

func (s *snapshotServer) Create(ctx context.Context, c *snapshot_rpc.CreateRequest) (*snapshot_rpc.CreateResponse, error) {
	sh, err := s.f.CreateSnapshot(ctx, common.UnmarshalCreationInfo(c))
	if err != nil {
		return nil, convertError(err)
	}

	return &snapshot_rpc.CreateResponse{
		Id: sh.GetID(),
	}, nil
}

func (s *snapshotServer) GetMap(r *snapshot_rpc.MapRequest, stream snapshot_rpc.SnapshotService_GetMapServer) error {
	buffer := make([]*snapshot_rpc.MapResponse_Chunk, 1000)

	var offset = int64(-1)
	for {
		cis, err := s.f.GetSnapshotChunksWithOffset(stream.Context(), r.Id, offset, len(buffer))
		if err != nil {
			return convertError(err)
		}
		if len(cis) == 0 {
			return nil
		}
		offset = cis[len(cis)-1].Offset

		for i, ci := range cis {
			buffer[i] = common.MarshalChunkInfo(ci)
		}

		if err = stream.Send(&snapshot_rpc.MapResponse{Chunks: buffer[:len(cis)]}); err != nil {
			return convertError(err)
		}
	}
}

func (s *snapshotServer) Read(ctx context.Context, r *snapshot_rpc.ReadRequest) (*snapshot_rpc.ReadResponse, error) {
	data, err := s.f.ReadChunkBody(ctx, r.Id, r.Coord.Offset)
	if err != nil {
		return nil, convertError(err)
	}
	resp := &snapshot_rpc.ReadResponse{Data: data}
	return resp, nil
}

func (s *snapshotServer) Write(ctx context.Context, w *snapshot_rpc.WriteRequest) (*snapshot_rpc.Status, error) {
	sh, err := s.f.OpenSnapshotWriteable(ctx, w.Id)
	if err != nil {
		return nil, convertError(err)
	}

	if w.Coord == nil {
		return nil, convertError(ErrInvalidCoord)
	}
	err = sh.Write(ctx, &chunker.StreamChunk{
		Offset:   w.Coord.Offset,
		Data:     w.Data,
		Progress: float64(w.Progress),
	})
	if err != nil {
		return nil, convertError(err)
	}

	return &snapshot_rpc.Status{}, nil
}

func (s *snapshotServer) Commit(ctx context.Context, c *snapshot_rpc.CommitRequest) (*snapshot_rpc.Status, error) {
	err := s.f.CommitSnapshot(ctx, c.Id, false)
	if err != nil {
		return nil, convertError(err)
	}
	return &snapshot_rpc.Status{}, nil
}

func (s *snapshotServer) Delete(ctx context.Context, d *snapshot_rpc.DeleteRequest) (*snapshot_rpc.Status, error) {
	err := s.f.DeleteSnapshot(ctx, common.UnmarshalDeleteRequest(d))
	if err != nil {
		return nil, convertError(err)
	}

	return &snapshot_rpc.Status{}, nil
}

func (s *snapshotServer) encodeList(_ context.Context, snapshots []common.SnapshotInfo, n int64) (*snapshot_rpc.SnapshotList, error) {
	l := &snapshot_rpc.SnapshotList{
		Result: make([]*snapshot_rpc.SnapshotInfo, 0, len(snapshots)),
	}
	for i := range snapshots {
		info, err := common.MarshalSnapshotInfo(&snapshots[i])
		if err != nil {
			return nil, convertError(err)
		}
		if p := storage.GetProgressFromDescription(info.State.Description); p != nil {
			info.State.Progress = *p
		}
		l.Result = append(l.Result, info)
	}

	// Pagination metadata
	// We request one extra record, drop it here.
	if n == 0 && len(snapshots) > defaultListLimit {
		l.Result = l.Result[:len(l.Result)-1]
		l.NextCursor = snapshots[len(snapshots)-2].ID
	}
	if n > 0 && int64(len(snapshots)) > n {
		l.Result = l.Result[:len(l.Result)-1]
		l.HasMore = true
	}

	return l, nil
}

func (s *snapshotServer) List(ctx context.Context, r *snapshot_rpc.ListRequest) (*snapshot_rpc.SnapshotList, error) {
	/* Two types of API:
	   1) ${Limit} = 0 + [optional Cursor] -> return ${defaultListLimit} records and NextCursor
	   2) ${Limit} in [1, ${maxListLimit}] + [optional Sort] -> return ${Limit} records and HasMore
	   Check request validity
	*/
	if r.Limit < 0 || r.Limit > maxListLimit {
		return nil, convertError(ErrInvalidLimit)
	}

	if r.Cursor != "" && (r.Limit > 0 || r.Sort != "") {
		return nil, convertError(ErrCursorAndSortLimit)
	}

	if r.Limit == 0 && r.Sort != "" {
		return nil, convertError(ErrSortWithoutLimit)
	}

	var err error
	req := &storage.ListRequest{
		N:            r.Limit + 1, // more records to find out whether more are available
		Last:         r.Cursor,
		Disk:         r.Disk,
		Project:      r.ProjectId,
		SearchPrefix: r.SearchPrefix,
		SearchFull:   r.SearchFull,
		Sort:         r.Sort,
	}

	if r.Limit == 0 {
		req.N = defaultListLimit + 1
	}

	if err = convertTS(r.BillingStart, &req.BillingStart); err != nil {
		return nil, convertError(err)
	}
	if err = convertTS(r.BillingEnd, &req.BillingEnd); err != nil {
		return nil, convertError(err)
	}
	if err = convertTS(r.CreatedAfter, &req.CreatedAfter); err != nil {
		return nil, convertError(err)
	}
	if err = convertTS(r.CreatedBefore, &req.CreatedBefore); err != nil {
		return nil, convertError(err)
	}

	snapshots, err := s.f.ListSnapshots(ctx, req)
	if err != nil {
		return nil, convertError(err)
	}

	return s.encodeList(ctx, snapshots, r.Limit)
}

func (s *snapshotServer) ListBases(ctx context.Context, r *snapshot_rpc.BaseListRequest) (*snapshot_rpc.SnapshotList, error) {
	req := &storage.BaseListRequest{
		N:  r.Limit,
		ID: r.Id,
	}

	snapshots, err := s.f.ListSnapshotBases(ctx, req)
	if err != nil {
		return nil, convertError(err)
	}

	// We do not support pagination here ATM
	return s.encodeList(ctx, snapshots, 0)
}

func (s *snapshotServer) ListChangedChildren(ctx context.Context, r *snapshot_rpc.ChangedChildrenListRequest) (*snapshot_rpc.ChangedChildrenList, error) {
	changedChildren, err := s.f.ListSnapshotChangedChildren(ctx, r.Id)
	if err != nil {
		return nil, convertError(err)
	}

	// We do not support pagination here ATM
	response, err := common.MarshalChangedChildrenList(changedChildren)
	if err != nil {
		return nil, convertError(err)
	}
	return response, nil
}

func (s *snapshotServer) Convert(ctx context.Context, c *snapshot_rpc.ConvertRequest) (*snapshot_rpc.ConvertResponse, error) {
	id, _, err := s.f.ConvertSnapshot(ctx, common.UnmarshalConvertRequest(c))
	if err != nil {
		return nil, convertError(err)
	}

	return &snapshot_rpc.ConvertResponse{
		Id: id,
	}, nil
}

func (s *snapshotServer) Info(ctx context.Context, req *snapshot_rpc.InfoRequest) (*snapshot_rpc.SnapshotInfo, error) {
	sh, err := s.f.OpenSnapshot(ctx, req.Id, req.SkipStatusCheck)
	if err != nil {
		return nil, convertError(err)
	}

	info, err := common.MarshalSnapshotInfo(sh.GetInfo())
	if err == nil {
		if p := storage.GetProgressFromDescription(info.State.Description); p != nil {
			info.State.Progress = *p
		}
	}
	return info, convertError(err)
}

func (s *snapshotServer) Update(ctx context.Context, req *snapshot_rpc.UpdateRequest) (*snapshot_rpc.Status, error) {
	r := common.UnmarshalUpdateRequest(req)
	if err := s.f.UpdateSnapshot(ctx, r); err != nil {
		return nil, convertError(err)
	}

	return &snapshot_rpc.Status{}, nil
}

func (s *snapshotServer) Ping(ctx context.Context, _ *snapshot_rpc.Status) (*snapshot_rpc.Status, error) {
	return &snapshot_rpc.Status{}, nil
}

func (s *snapshotServer) Verify(ctx context.Context, req *snapshot_rpc.VerifyRequest) (*snapshot_rpc.VerifyReport, error) {
	report, err := s.f.VerifyChecksum(ctx, req.Id, req.Full)
	if err != nil {
		return nil, convertError(err)
	}

	return common.MarshalVerifyReport(report), nil
}

func (s *snapshotServer) Copy(ctx context.Context, req *snapshot_rpc.CopyRequest) (*snapshot_rpc.CopyResponse, error) {
	id, err := s.f.ShallowCopy(ctx, common.UnmarshalCopyRequest(req))
	if err != nil {
		return nil, convertError(err)
	}

	return &snapshot_rpc.CopyResponse{Id: id}, nil
}

func (s *snapshotServer) Move(ctx context.Context, req *snapshot_rpc.MoveRequest) (*snapshot_rpc.Status, error) {
	err := s.f.Move(ctx, common.UnmarshalMoveRequest(req))
	if err != nil {
		return nil, convertError(err)
	}

	return &snapshot_rpc.Status{}, nil
}

func (s *snapshotServer) ListTasks(ctx context.Context, req *snapshot_rpc.ListTasksRequest) (*snapshot_rpc.ListTasksResponse, error) {
	l := s.f.ListTasks(ctx)
	for _, t := range l {
		if t.Status.Error != nil {
			t.Status.Error = convertError(t.Status.Error)
		}
	}

	return common.MarshalTaskInfoList(l)
}

func (s *snapshotServer) GetTask(ctx context.Context, req *snapshot_rpc.GetTaskRequest) (*snapshot_rpc.TaskData, error) {
	info, err := s.f.GetTask(ctx, req.TaskId)
	if err != nil {
		return nil, convertError(err)
	}
	if info.Status.Error != nil {
		info.Status.Error = convertError(err)
	}

	return common.MarshalTaskData(info)
}

func (s *snapshotServer) GetTaskStatus(ctx context.Context, req *snapshot_rpc.GetTaskStatusRequest) (*snapshot_rpc.TaskStatus, error) {
	ts, err := s.f.GetTaskStatus(ctx, req.TaskId)
	if err != nil {
		return nil, convertError(err)
	}
	if ts.Error != nil {
		ts.Error = convertError(ts.Error)
	}

	return common.MarshalTaskStatus(ts)
}

func (s *snapshotServer) CancelTask(ctx context.Context, req *snapshot_rpc.CancelTaskRequest) (*snapshot_rpc.Status, error) {
	err := s.f.CancelTask(ctx, req.TaskId)
	return &snapshot_rpc.Status{}, convertError(err)
}

func (s *snapshotServer) DeleteTask(ctx context.Context, req *snapshot_rpc.DeleteTaskRequest) (*snapshot_rpc.Status, error) {
	err := s.f.DeleteTask(ctx, req.TaskId)
	return &snapshot_rpc.Status{}, convertError(err)
}

func (s *snapshotServer) Close() error {
	return convertError(s.f.Close(s.Context))
}
