package client

import (
	"crypto/tls"
	"fmt"
	"time"

	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"

	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/globalauth"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/snapshot_rpc"
)

const maxMsgSize = 100 * 1024 * 1024

// List is a list of snapshot metadata.
type List struct {
	Result     []*common.SnapshotInfo
	NextCursor string
	HasMore    bool
}

// Client is a snapshot service client.
type Client struct {
	conn      snapshot_rpc.SnapshotServiceClient
	transport *grpc.ClientConn
}

// NewClient creates a new snapshot service client.
func NewClient(ctx context.Context, endpoint string, secure bool, provider globalauth.TokenProvider) (*Client, error) {
	opts := []grpc.DialOption{
		grpc.WithDefaultCallOptions(
			grpc.MaxCallSendMsgSize(maxMsgSize),
			grpc.MaxCallRecvMsgSize(maxMsgSize),
		)}
	if secure {
		opts = append(opts, grpc.WithTransportCredentials(credentials.NewTLS(&tls.Config{})))
	} else {
		opts = append(opts, grpc.WithInsecure())
	}
	if provider != nil {
		opts = append(opts, grpc.WithPerRPCCredentials(&globalauth.GRPCTokenProvider{
			TokenProvider: provider,
			Secure:        secure,
		}))
	}
	return New(ctx, endpoint, opts...)
}

// New creates a client, but in a more flexible way than NewClient
func New(ctx context.Context, target string, opts ...grpc.DialOption) (*Client, error) {
	conn, err := grpc.DialContext(ctx, target, opts...)
	if err != nil {
		return nil, err
	}
	return &Client{conn: snapshot_rpc.NewSnapshotServiceClient(conn), transport: conn}, nil
}

// Ping calls ping handler of the service
func (c *Client) Ping(ctx context.Context, opts ...grpc.CallOption) error {
	pingRequest := new(snapshot_rpc.Status)
	_, err := c.conn.Ping(ctx, pingRequest, opts...)
	return err
}

// PingCheck checks snapshot service availability.
// CLOUD-2632: to be used for monitoring snapshot availability from template
func PingCheck(endpoint string) error {
	ctx, cancel := context.WithTimeout(context.Background(), time.Second*10)
	defer cancel()

	opts := []grpc.DialOption{grpc.WithBlock(), grpc.WithInsecure()}
	conn, err := grpc.DialContext(ctx, endpoint, opts...)
	if err != nil {
		return err
	}
	return conn.Close()
}

// CreateSnapshot creates a snapshot with given metadata.
func (c *Client) CreateSnapshot(ctx context.Context, i *common.CreationInfo) (string, error) {
	cr, err := c.conn.Create(
		ctx,
		common.MarshalCreationInfo(i))
	if err != nil {
		return "", err
	}

	return cr.Id, nil
}

func (c *Client) MoveSnapshot(ctx context.Context, m *common.MoveRequest) error {
	req, err := common.MarshalMoveRequest(m)
	if err != nil {
		return err
	}

	_, err = c.conn.Move(ctx, req)
	return err
}

func (c *Client) CopySnapshot(ctx context.Context, m *common.CopyRequest) error {
	_, err := c.conn.Copy(ctx, common.MarshalCopyRequest(m))
	return err
}

// OpenSnapshot opens a snapshot for reading.
func (c *Client) OpenSnapshot(ctx context.Context, id string) (SnapshotReader, error) {
	return newSnapshotReader(ctx, c.conn, id)
}

// WriteSnapshot puts a chunk of data to snapshot.
func (c *Client) WriteSnapshot(ctx context.Context, id string, b []byte, offset int64) error {
	return c.WriteSnapshotWithProgress(ctx, id, b, offset, 0)
}

// WriteSnapshotWithProgress puts a chunk of data to snapshot with custom progress percent.
func (c *Client) WriteSnapshotWithProgress(ctx context.Context, id string, b []byte, offset int64, progress float32) error {
	_, err := c.conn.Write(
		ctx,
		&snapshot_rpc.WriteRequest{
			Id: id,
			Coord: &snapshot_rpc.Coord{
				Offset:    offset,
				Chunksize: int64(len(b)),
			},
			Data:     b,
			Progress: progress,
		})
	return err
}

// CommitSnapshot finishes snapshot creation and makes it readable.
func (c *Client) CommitSnapshot(ctx context.Context, id string) error {
	_, err := c.conn.Commit(
		ctx,
		&snapshot_rpc.CommitRequest{
			Id: id,
		})
	return err
}

// DeleteSnapshot deletes a snapshot.
func (c *Client) DeleteSnapshot(ctx context.Context, d *common.DeleteRequest) error {
	req, err := common.MarshalDeleteRequest(d)
	if err != nil {
		return err
	}
	_, err = c.conn.Delete(ctx, req)
	return err
}

// ListQueryArg is functional option pattern to specify request arguments
type ListQueryArg func(*snapshot_rpc.ListRequest) error

// ListDiskArg adds disk specification to List
var ListDiskArg = func(disk string) ListQueryArg {
	return func(r *snapshot_rpc.ListRequest) error {
		r.Disk = disk
		return nil
	}
}

// ListProjectArg adds project specification to List
var ListProjectArg = func(project string) ListQueryArg {
	return func(r *snapshot_rpc.ListRequest) error {
		r.ProjectId = project
		return nil
	}
}

// ListLimitArg adds limit option
var ListLimitArg = func(limit int64) ListQueryArg {
	return func(r *snapshot_rpc.ListRequest) error {
		r.Limit = limit
		return nil
	}
}

// ListCursorArg adds cursor option
var ListCursorArg = func(cursor string) ListQueryArg {
	return func(r *snapshot_rpc.ListRequest) error {
		r.Cursor = cursor
		return nil
	}
}

// ListSortArg adds sort option
var ListSortArg = func(sort string) ListQueryArg {
	return func(r *snapshot_rpc.ListRequest) error {
		r.Sort = sort
		return nil
	}
}

// ListSubstrArg adds substring search option
var ListSubstrArg = func(sub string) ListQueryArg {
	return func(r *snapshot_rpc.ListRequest) error {
		r.SearchFull = sub
		return nil
	}
}

// ListSnapshots queries snapshots using provided args as filter
func (c *Client) ListSnapshots(ctx context.Context, args ...ListQueryArg) (List, error) {
	req := new(snapshot_rpc.ListRequest)
	for _, arg := range args {
		if err := arg(req); err != nil {
			return List{}, err
		}
	}

	l, err := c.conn.List(ctx, req)
	if err != nil {
		return List{}, err
	}

	s := make([]*common.SnapshotInfo, 0, len(l.Result))
	for _, sh := range l.Result {
		i, err := common.UnmarshalSnapshotInfo(sh)
		if err != nil {
			return List{}, err
		}
		s = append(s, i)
	}

	return List{
		Result:     s,
		NextCursor: l.NextCursor,
		HasMore:    l.HasMore,
	}, nil
}

// ListSnapshotsFull returns a full list of snapshots matching the request.
func (c *Client) ListSnapshotsFull(ctx context.Context, args ...ListQueryArg) ([]*common.SnapshotInfo, error) {
	// check args validity
	req := new(snapshot_rpc.ListRequest)
	for _, arg := range args {
		if err := arg(req); err != nil {
			return nil, err
		}
	}
	if req.Limit != 0 || req.Sort != "" {
		return nil, fmt.Errorf("limit and Sort cannot be used in ListSnapshotsFull")
	}

	var total []*common.SnapshotInfo
	for {
		l, err := c.ListSnapshots(ctx, args...)
		if err != nil {
			return nil, err
		}

		total = append(total, l.Result...)

		if l.NextCursor == "" {
			break
		}
		args = append(args, ListCursorArg(l.NextCursor))
	}
	return total, nil
}

// ConvertSnapshot starts a snapshot conversion operation.
func (c *Client) ConvertSnapshot(ctx context.Context, req *common.ConvertRequest) (string, error) {
	r, err := c.conn.Convert(ctx, common.MarshalConvertRequest(req))
	if err != nil {
		return "", err
	}

	return r.Id, nil
}

// GetSnapshotInfo returns metadata of a specific snapshot.
func (c *Client) GetSnapshotInfo(ctx context.Context, id string) (*common.SnapshotInfo, error) {
	r, err := c.conn.Info(
		ctx,
		&snapshot_rpc.InfoRequest{
			Id: id,
		})
	if err != nil {
		return nil, err
	}

	return common.UnmarshalSnapshotInfo(r)
}

// UpdateSnapshot updates specific fields in snapshot.
func (c *Client) UpdateSnapshot(ctx context.Context, d *common.UpdateRequest) error {
	_, err := c.conn.Update(ctx, common.MarshalUpdateRequest(d))
	return err
}

// ListTasks shows all tasks that are in snapshot task manager now
func (c *Client) ListTasks(ctx context.Context) ([]*common.TaskInfo, error) {
	tasks, err := c.conn.ListTasks(ctx, &snapshot_rpc.ListTasksRequest{})
	if err != nil {
		return nil, err
	}
	return common.UnmarshalTaskInfoList(tasks)
}

// GetTask returns task information
func (c *Client) GetTask(ctx context.Context, taskID string) (*common.TaskData, error) {
	status, err := c.conn.GetTask(
		ctx,
		&snapshot_rpc.GetTaskRequest{
			TaskId: taskID,
		})
	if err != nil {
		return nil, err
	}
	return common.UnmarshalTaskData(status)
}

// GetTaskStatus gets task status
func (c *Client) GetTaskStatus(ctx context.Context, taskID string) (*common.TaskStatus, error) {
	status, err := c.conn.GetTaskStatus(
		ctx,
		&snapshot_rpc.GetTaskStatusRequest{
			TaskId: taskID,
		})
	if err != nil {
		return nil, err
	}
	return common.UnmarshalTaskStatus(status)
}

// CancelTask cancels task on snapshot
func (c *Client) CancelTask(ctx context.Context, taskID string) error {
	_, err := c.conn.CancelTask(
		ctx,
		&snapshot_rpc.CancelTaskRequest{
			TaskId: taskID,
		})
	return err
}

// DeleteTask deletes task on snapshot
func (c *Client) DeleteTask(ctx context.Context, taskID string) error {
	_, err := c.conn.DeleteTask(
		ctx,
		&snapshot_rpc.DeleteTaskRequest{
			TaskId: taskID,
		})
	return err
}

// Close closes the connection.
func (c *Client) Close() error {
	return c.transport.Close()
}
