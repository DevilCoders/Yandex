package snapshot

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"math/rand"
	"time"

	"google.golang.org/grpc"
	grpc_codes "google.golang.org/grpc/codes"
	"google.golang.org/grpc/credentials"
	grpc_status "google.golang.org/grpc/status"

	nbs_client "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	snapshot_client "a.yandex-team.ru/cloud/compute/snapshot/snapshot/client"
	snapshot_common "a.yandex-team.ru/cloud/compute/snapshot/snapshot/common"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/auth"
	snapshot_config "a.yandex-team.ru/cloud/disk_manager/internal/pkg/clients/snapshot/config"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/monitoring/metrics"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/tasks/errors"
	error_codes "a.yandex-team.ru/cloud/disk_manager/pkg/client/codes"
)

////////////////////////////////////////////////////////////////////////////////

const maxStatusFailuresBeforeRediscover = 10

////////////////////////////////////////////////////////////////////////////////

func imageName(imageID string) string {
	return fmt.Sprintf("-image-%s", imageID)
}

func snapshotName(snapshotID string) string {
	return fmt.Sprintf("-snapshot-%s", snapshotID)
}

func toMilliseconds(duration time.Duration) int32 {
	return int32(duration / time.Millisecond)
}

////////////////////////////////////////////////////////////////////////////////

func isRetriableScheduleError(requestCtx context.Context, e error) bool {
	if requestCtx.Err() != nil {
		// Unable to retry when request was cancelled by us.
		return false
	}

	status, ok := grpc_status.FromError(e)
	if !ok {
		return false
	}

	return status.Code() == grpc_codes.DeadlineExceeded ||
		status.Code() == grpc_codes.Canceled ||
		status.Code() == grpc_codes.Unavailable
}

func logRetriableScheduleError(
	ctx context.Context,
	endpoint string,
	taskID string,
	e error,
) {

	message := "Got retriable schedule error from snapshot %v while executing task %v: %v"

	status, ok := grpc_status.FromError(e)
	if ok && status.Code() == grpc_codes.Unavailable {
		logging.Debug(ctx, message, endpoint, taskID, e)
	} else {
		logging.Warn(ctx, message, endpoint, taskID, e)
	}
}

////////////////////////////////////////////////////////////////////////////////

func isRetriableStatusError(requestCtx context.Context, e error) bool {
	if requestCtx.Err() != nil {
		// Unable to retry when request was cancelled by us.
		return false
	}

	status, ok := grpc_status.FromError(e)
	if !ok {
		return false
	}

	return status.Code() == grpc_codes.NotFound ||
		status.Code() == grpc_codes.DeadlineExceeded ||
		status.Code() == grpc_codes.Canceled ||
		status.Code() == grpc_codes.Unavailable
}

////////////////////////////////////////////////////////////////////////////////

func isRetriableSnapshotError(status *grpc_status.Status) bool {
	var snapshotErr snapshot_common.SnapshotError
	err := json.Unmarshal([]byte(status.Message()), &snapshotErr)
	if err != nil {
		return false
	}

	if snapshotErr.Code == "ErrNbs" {
		var nbsErr nbs_client.ClientError
		err = json.Unmarshal([]byte(snapshotErr.Message), &nbsErr)
		if err != nil {
			return false
		}

		// MountVolume error.
		return nbsErr.Code == nbs_client.E_ARGUMENT || // for back compatibility
			nbsErr.Code == nbs_client.E_MOUNT_CONFLICT
	}

	return false
}

func isRetriableExecuteError(requestCtx context.Context, e error) bool {
	if requestCtx.Err() != nil {
		// Unable to retry when request was cancelled by us.
		return false
	}

	status, ok := grpc_status.FromError(e)
	if !ok {
		return false
	}

	return isRetriableSnapshotError(status) ||
		status.Code() == grpc_codes.DeadlineExceeded ||
		status.Code() == grpc_codes.Canceled ||
		status.Code() == grpc_codes.Unavailable
}

func wrapExecuteError(e error, endpoint string) error {
	status, ok := grpc_status.FromError(e)
	e = fmt.Errorf("got execute error from %v: %w", endpoint, e)

	if !ok {
		return e
	}

	var snapshotErr snapshot_common.SnapshotError
	err := json.Unmarshal([]byte(status.Message()), &snapshotErr)
	if err != nil {
		return e
	}

	if snapshotErr.Public {
		// TODO: It's strange that we interpret all public errors from snapshot
		// as BadSource error.
		return &errors.DetailedError{
			Err: e,
			Details: &errors.ErrorDetails{
				Code:     error_codes.BadSource,
				Message:  snapshotErr.Message,
				Internal: false,
			},
		}
	}

	if snapshotErr.Code == "ErrNbs" {
		var nbsErr nbs_client.ClientError
		err = json.Unmarshal([]byte(snapshotErr.Message), &nbsErr)
		if err != nil {
			return e
		}

		// TODO: remove support for PathDoesNotExist
		if (nbsErr.Facility() == nbs_client.FACILITY_SCHEMESHARD && nbsErr.Status() == 2) ||
			nbsErr.Code == nbs_client.E_NOT_FOUND {

			return &errors.NonRetriableError{
				Err:    e,
				Silent: true,
			}
		}

		// Yet another hacks (NBS-2401, NBS-2416).
		if nbsErr.Code == nbs_client.E_CANCELLED ||
			nbsErr.Code == nbs_client.E_INVALID_SESSION {

			return &errors.RetriableError{Err: e}
		}

		if !nbsErr.IsRetriable() {
			return e
		}
	}

	// NBS-1930: HACK: some errors should not be retriable.
	return &errors.RetriableError{Err: e}
}

////////////////////////////////////////////////////////////////////////////////

func wrapSnapshotInfoError(e error, endpoint string) error {
	status, ok := grpc_status.FromError(e)
	e = fmt.Errorf("got GetSnapshotInfo error from %v: %w", endpoint, e)

	if !ok {
		return e
	}

	var snapshotErr snapshot_common.SnapshotError
	err := json.Unmarshal([]byte(status.Message()), &snapshotErr)
	if err == nil {
		if snapshotErr.Public {
			// TODO: it's strange that we interpret all public errors from snapshot
			// as BadSource error.
			return &errors.DetailedError{
				Err: e,
				Details: &errors.ErrorDetails{
					Code:     error_codes.BadSource,
					Message:  snapshotErr.Message,
					Internal: false,
				},
			}
		}
	}

	// TODO: it's strange that we interpret all non-public errors as retriable.
	return &errors.RetriableError{Err: e}
}

////////////////////////////////////////////////////////////////////////////////

func isSnapshotNotFoundError(e error) bool {
	status, ok := grpc_status.FromError(e)
	if !ok {
		return false
	}

	var snapshotErr snapshot_common.SnapshotError
	err := json.Unmarshal([]byte(status.Message()), &snapshotErr)
	if err != nil {
		return false
	}

	return snapshotErr.Code == "ErrSnapshotNotFound"
}

func isAlreadyCreatedError(e error) bool {
	status, ok := grpc_status.FromError(e)
	if !ok {
		return false
	}

	var snapshotErr snapshot_common.SnapshotError
	err := json.Unmarshal([]byte(status.Message()), &snapshotErr)
	if err != nil {
		return false
	}

	// TODO: Should check that already existing snapshot is the one that we have
	// created.
	return snapshotErr.Code == "ErrSnapshotReadOnly" || snapshotErr.Code == "ErrDuplicateID"
}

func isAlreadyExistsError(e error) bool {
	status, ok := grpc_status.FromError(e)
	if !ok {
		return false
	}

	return status.Code() == grpc_codes.AlreadyExists
}

////////////////////////////////////////////////////////////////////////////////

func newSnapshot(
	ctx context.Context,
	endpoint string,
	config *snapshot_config.ClientConfig,
) (*snapshot_client.Client, error) {

	opts := []grpc.DialOption{}

	if config.GetSecure() {
		cfg := tls.Config{
			MinVersion: tls.VersionTLS12,
		}

		if len(config.GetRootCertsFile()) != 0 {
			pem, err := ioutil.ReadFile(config.GetRootCertsFile())
			if err != nil {
				return nil, &errors.NonRetriableError{
					Err: fmt.Errorf("failed to read root certs file: %v", err),
				}
			}

			pool := x509.NewCertPool()
			ok := pool.AppendCertsFromPEM(pem)
			if !ok {
				return nil, &errors.NonRetriableError{
					Err: fmt.Errorf("failed to parse PEM"),
				}
			}

			cfg.RootCAs = pool
		}

		opts = append(opts, grpc.WithTransportCredentials(credentials.NewTLS(&cfg)))
	} else {
		opts = append(opts, grpc.WithInsecure())
	}

	snapshot, err := snapshot_client.New(ctx, endpoint, opts...)
	if err != nil {
		return nil, &errors.RetriableError{Err: err}
	}

	return snapshot, nil
}

func newClient(
	ctx context.Context,
	config *snapshot_config.ClientConfig,
	zoneID string,
	creds auth.Credentials,
	metrics clientMetrics,
) (Client, error) {

	zone, ok := config.GetZones()[zoneID]
	if !ok {
		return nil, &errors.NonRetriableError{
			Err: fmt.Errorf(
				"failed to find zone %v in config.Zones %v",
				zoneID,
				config.GetZones(),
			),
		}
	}

	if len(zone.Endpoints) == 0 {
		return nil, &errors.NonRetriableError{
			Err: fmt.Errorf(
				"at least one snapshot endpoint should be specified for zone %v",
				zoneID,
			),
		}
	}

	// Pick random snapshot endpoint.
	rand.Seed(time.Now().UnixNano())
	endpointIdx := rand.Intn(len(zone.Endpoints))

	snapshot, err := newSnapshot(ctx, zone.Endpoints[endpointIdx], config)
	if err != nil {
		return nil, err
	}

	return &client{
		snapshot:    snapshot,
		config:      config,
		credentials: creds,
		endpoints:   zone.Endpoints,
		endpointIdx: endpointIdx,
		metrics:     metrics,
	}, nil
}

////////////////////////////////////////////////////////////////////////////////

type clientMetrics struct {
	registry metrics.Registry
	errors   metrics.CounterVec
}

func (m *clientMetrics) OnError(e error) {
	status, ok := grpc_status.FromError(e)
	if ok {
		var snapshotErr snapshot_common.SnapshotError
		err := json.Unmarshal([]byte(status.Message()), &snapshotErr)
		if err == nil && len(snapshotErr.Code) != 0 {
			m.errors.With(map[string]string{"code": snapshotErr.Code}).Inc()
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

type client struct {
	snapshot    *snapshot_client.Client
	config      *snapshot_config.ClientConfig
	credentials auth.Credentials
	endpoints   []string
	endpointIdx int
	metrics     clientMetrics
}

func (c *client) discoverRoundRobin(
	ctx context.Context,
) error {

	newEndpointIdx := (c.endpointIdx + 1) % len(c.endpoints)
	snapshot, err := newSnapshot(
		ctx,
		c.endpoints[newEndpointIdx],
		c.config,
	)
	if err != nil {
		return err
	}

	// Close previous snapshot client.
	_ = c.snapshot.Close()
	c.snapshot = snapshot
	c.endpointIdx = newEndpointIdx
	return nil
}

func (c *client) getTaskStatus(
	ctx context.Context,
	taskID string,
) (TaskStatus, error) {

	reqCtx, err := c.getRequestContext(ctx)
	if err != nil {
		return TaskStatus{}, err
	}

	status, err := c.snapshot.GetTaskStatus(reqCtx, taskID)
	if err != nil {
		return TaskStatus{}, err
	}

	statusError := status.Error
	if statusError == nil && !status.Success {
		statusError = fmt.Errorf(
			"status.Success should be true when status.Error is empty",
		)
	}

	return TaskStatus{
		Finished: status.Finished,
		Progress: status.Progress,
		Offset:   status.Offset,
		Error:    statusError,
	}, nil
}

func (c *client) getRequestContext(
	ctx context.Context,
) (context.Context, error) {

	if c.credentials != nil {
		token, err := c.credentials.Token(ctx)
		if err != nil {
			return nil, err
		}

		return headers.SetOutgoingAccessToken(ctx, token), nil
	}

	return ctx, nil
}

func (c *client) executeImpl(
	ctx context.Context,
	state *TaskState,
	schedule func(context.Context) error,
	saveState SaveStateFunc,
	isAlreadyFinished func(error) bool,
) error {

	statusFailures := 0
	rediscoverCount := 0

	rediscover := func() error {
		if rediscoverCount >= len(c.endpoints) {
			return &errors.RetriableError{
				Err: fmt.Errorf("maximum number of rediscovers exceeded"),
			}
		}

		err := c.discoverRoundRobin(ctx)
		if err != nil {
			return err
		}

		rediscoverCount++
		statusFailures = 0
		<-time.After(state.BackoffTimeout)
		return nil
	}

	var cleanup func() error

	for {
		cleanup = func() error {
			reqCtx, err := c.getRequestContext(ctx)
			if err != nil {
				return err
			}

			err = c.snapshot.DeleteTask(reqCtx, state.TaskID)
			if err == nil {
				return nil
			}

			status, ok := grpc_status.FromError(err)
			if !ok || status.Code() != grpc_codes.NotFound {
				logging.Warn(
					ctx,
					"Got error from snapshot %v when deleting task %v: %v",
					c.endpoints[c.endpointIdx],
					state.TaskID,
					err,
				)
			}

			return err
		}
		defer cleanup()

		reqCtx, err := c.getRequestContext(ctx)
		if err != nil {
			return err
		}

		err = schedule(reqCtx)
		if err != nil {
			if isAlreadyFinished(err) {
				return nil
			}

			if !isAlreadyExistsError(err) {
				if isRetriableScheduleError(reqCtx, err) {
					logRetriableScheduleError(
						ctx,
						c.endpoints[c.endpointIdx],
						state.TaskID,
						err,
					)

					err := rediscover()
					if err != nil {
						return err
					}
					// Go to start and retry.
					continue
				}

				// NBS-2141: local retries have failed, task should be restarted.
				return &errors.RetriableError{Err: err}
			}
		}

		for {
			status, err := c.getTaskStatus(ctx, state.TaskID)
			if err != nil {
				if isRetriableStatusError(ctx, err) {
					logging.Warn(
						ctx,
						"Got retriable status error from snapshot %v while executing task %v: %v",
						c.endpoints[c.endpointIdx],
						state.TaskID,
						err,
					)

					statusFailures++
					if statusFailures == maxStatusFailuresBeforeRediscover {
						err := rediscover()
						if err != nil {
							return err
						}
					} else {
						<-time.After(state.PingPeriod)
					}
					// Go to start and retry.
					break
				}

				// NBS-2563: local retries have failed, task should be restarted.
				return &errors.RetriableError{Err: err}
			}

			if status.Finished {
				if status.Error != nil {
					if isAlreadyFinished(status.Error) {
						return nil
					}

					if isRetriableExecuteError(reqCtx, status.Error) {
						logging.Warn(
							ctx,
							"Got retriable execute error from snapshot %v while executing task %v: %v",
							c.endpoints[c.endpointIdx],
							state.TaskID,
							status.Error,
						)

						// Make snapshot forget about this task error.
						// Not really interested in error.
						_ = cleanup()

						err := rediscover()
						if err != nil {
							return err
						}
						// Go to start and retry.
						break
					}

					c.metrics.OnError(status.Error)
					return wrapExecuteError(
						status.Error,
						c.endpoints[c.endpointIdx],
					)
				}

				return nil
			}

			// NBS-2399: offset cannot run backward.
			if status.Offset > state.Offset {
				err = saveState(status.Offset, status.Progress)
				if err != nil {
					return err
				}
				state.Offset = status.Offset
			}

			<-time.After(state.PingPeriod)
		}
	}
}

func (c *client) execute(
	ctx context.Context,
	state *TaskState,
	schedule func(context.Context) error,
	saveState SaveStateFunc,
) error {

	return c.executeImpl(
		ctx,
		state,
		schedule,
		saveState,
		func(err error) bool {
			return false
		},
	)
}

func (c *client) createImageFromURL(
	ctx context.Context,
	state *CreateImageFromURLState,
) error {

	return c.snapshot.MoveSnapshot(ctx, &snapshot_common.MoveRequest{
		Src: &snapshot_common.URLMoveSrc{
			URL:    state.SrcURL,
			Format: state.Format,
		},
		Dst: &snapshot_common.SnapshotMoveDst{
			SnapshotID: state.DstImageID,
			ImageID:    state.DstImageID,
			ProjectID:  state.FolderID,
			Create:     true,
			Commit:     true,
			Name:       imageName(state.DstImageID),
		},
		Params: snapshot_common.MoveParams{
			Offset:             state.State.Offset,
			BlockSize:          int64(c.config.GetBlockSize()),
			TaskID:             state.State.TaskID,
			HeartbeatTimeoutMs: toMilliseconds(state.State.HeartbeatTimeout),
			HeartbeatEnabled:   true,
			SkipZeroes:         true,
			SkipNonexistent:    true,
		},
		OperationCloudID:   state.OperationCloudID,
		OperationProjectID: state.OperationFolderID,
	})
}

func (c *client) createImageFromImage(
	ctx context.Context,
	state *CreateImageFromImageState,
) error {

	return c.snapshot.CopySnapshot(ctx, &snapshot_common.CopyRequest{
		ID:              state.SrcImageID,
		TargetID:        state.DstImageID,
		TargetProjectID: state.FolderID,
		Name:            imageName(state.DstImageID),
		ImageID:         state.DstImageID,
		Params: snapshot_common.CopyParams{
			TaskID:             state.State.TaskID,
			HeartbeatTimeoutMs: toMilliseconds(state.State.HeartbeatTimeout),
			HeartbeatEnabled:   true,
		},
		OperationCloudID:   state.OperationCloudID,
		OperationProjectID: state.OperationFolderID,
	})
}

func (c *client) createImageFromSnapshot(
	ctx context.Context,
	state *CreateImageFromSnapshotState,
) error {

	return c.snapshot.CopySnapshot(ctx, &snapshot_common.CopyRequest{
		ID:              state.SrcSnapshotID,
		TargetID:        state.DstImageID,
		TargetProjectID: state.FolderID,
		Name:            imageName(state.DstImageID),
		ImageID:         state.DstImageID,
		Params: snapshot_common.CopyParams{
			TaskID:             state.State.TaskID,
			HeartbeatTimeoutMs: toMilliseconds(state.State.HeartbeatTimeout),
			HeartbeatEnabled:   true,
		},
		OperationCloudID:   state.OperationCloudID,
		OperationProjectID: state.OperationFolderID,
	})
}

func (c *client) createImageFromDisk(
	ctx context.Context,
	state *CreateImageFromDiskState,
) error {

	return c.snapshot.MoveSnapshot(ctx, &snapshot_common.MoveRequest{
		Src: &snapshot_common.NbsMoveSrc{
			ClusterID:        state.SrcDisk.ZoneId,
			DiskID:           state.SrcDisk.DiskId,
			LastCheckpointID: state.SrcDiskCheckpointID,
		},
		Dst: &snapshot_common.SnapshotMoveDst{
			SnapshotID: state.DstImageID,
			ProjectID:  state.FolderID,
			Create:     true,
			Commit:     true,
			Name:       imageName(state.DstImageID),
		},
		Params: snapshot_common.MoveParams{
			Offset:             state.State.Offset,
			BlockSize:          int64(c.config.GetBlockSize()),
			TaskID:             state.State.TaskID,
			HeartbeatTimeoutMs: toMilliseconds(state.State.HeartbeatTimeout),
			HeartbeatEnabled:   true,
			SkipZeroes:         true,
			SkipNonexistent:    true,
		},
		OperationCloudID:   state.OperationCloudID,
		OperationProjectID: state.OperationFolderID,
	})
}

func (c *client) createSnapshotFromDisk(
	ctx context.Context,
	state *CreateSnapshotFromDiskState,
) error {

	return c.snapshot.MoveSnapshot(ctx, &snapshot_common.MoveRequest{
		Src: &snapshot_common.NbsMoveSrc{
			ClusterID:         state.SrcDisk.ZoneId,
			DiskID:            state.SrcDisk.DiskId,
			FirstCheckpointID: state.SrcDiskBaseCheckpointID,
			LastCheckpointID:  state.SrcDiskCheckpointID,
		},
		Dst: &snapshot_common.SnapshotMoveDst{
			SnapshotID:     state.DstSnapshotID,
			ProjectID:      state.FolderID,
			Create:         true,
			Commit:         true,
			Name:           snapshotName(state.DstSnapshotID),
			BaseSnapshotID: state.DstBaseSnapshotID,
		},
		Params: snapshot_common.MoveParams{
			Offset:             state.State.Offset,
			BlockSize:          int64(c.config.GetBlockSize()),
			TaskID:             state.State.TaskID,
			HeartbeatTimeoutMs: toMilliseconds(state.State.HeartbeatTimeout),
			HeartbeatEnabled:   true,
			SkipZeroes:         true,
			SkipNonexistent:    true,
		},
		OperationCloudID:   state.OperationCloudID,
		OperationProjectID: state.OperationFolderID,
	})
}

func (c *client) transferFromImageToDisk(
	ctx context.Context,
	state *TransferFromImageToDiskState,
) error {

	return c.snapshot.MoveSnapshot(ctx, &snapshot_common.MoveRequest{
		Src: &snapshot_common.SnapshotMoveSrc{
			SnapshotID: state.SrcImageID,
		},
		Dst: &snapshot_common.NbsMoveDst{
			ClusterID: state.DstDisk.ZoneId,
			DiskID:    state.DstDisk.DiskId,
			Mount:     true,
		},
		Params: snapshot_common.MoveParams{
			Offset:             state.State.Offset,
			BlockSize:          int64(c.config.GetBlockSize()),
			TaskID:             state.State.TaskID,
			HeartbeatTimeoutMs: toMilliseconds(state.State.HeartbeatTimeout),
			HeartbeatEnabled:   true,
			SkipZeroes:         true,
			SkipNonexistent:    true,
		},
		OperationCloudID:   state.OperationCloudID,
		OperationProjectID: state.OperationFolderID,
	})
}

func (c *client) transferFromSnapshotToDisk(
	ctx context.Context,
	state *TransferFromSnapshotToDiskState,
) error {

	return c.snapshot.MoveSnapshot(ctx, &snapshot_common.MoveRequest{
		Src: &snapshot_common.SnapshotMoveSrc{
			SnapshotID: state.SrcSnapshotID,
		},
		Dst: &snapshot_common.NbsMoveDst{
			ClusterID: state.DstDisk.ZoneId,
			DiskID:    state.DstDisk.DiskId,
			Mount:     true,
		},
		Params: snapshot_common.MoveParams{
			Offset:             state.State.Offset,
			BlockSize:          int64(c.config.GetBlockSize()),
			TaskID:             state.State.TaskID,
			HeartbeatTimeoutMs: toMilliseconds(state.State.HeartbeatTimeout),
			HeartbeatEnabled:   true,
			SkipZeroes:         true,
			SkipNonexistent:    true,
		},
		OperationCloudID:   state.OperationCloudID,
		OperationProjectID: state.OperationFolderID,
	})
}

func (c *client) deleteImage(
	ctx context.Context,
	state *DeleteImageState,
) error {

	return c.snapshot.DeleteSnapshot(ctx, &snapshot_common.DeleteRequest{
		ID:              state.ImageID,
		SkipStatusCheck: true,
		Params: snapshot_common.DeleteParams{
			TaskID:             state.State.TaskID,
			HeartbeatTimeoutMs: toMilliseconds(state.State.HeartbeatTimeout),
			HeartbeatEnabled:   true,
		},
		OperationCloudID:   state.OperationCloudID,
		OperationProjectID: state.OperationFolderID,
	})
}

func (c *client) deleteSnapshot(
	ctx context.Context,
	state *DeleteSnapshotState,
) error {

	return c.snapshot.DeleteSnapshot(ctx, &snapshot_common.DeleteRequest{
		ID:              state.SnapshotID,
		SkipStatusCheck: true,
		Params: snapshot_common.DeleteParams{
			TaskID:             state.State.TaskID,
			HeartbeatTimeoutMs: toMilliseconds(state.State.HeartbeatTimeout),
			HeartbeatEnabled:   true,
		},
		OperationCloudID:   state.OperationCloudID,
		OperationProjectID: state.OperationFolderID,
	})
}

func (c *client) Close() error {
	return c.snapshot.Close()
}

func (c *client) CreateImageFromURL(
	ctx context.Context,
	state CreateImageFromURLState,
	saveState SaveStateFunc,
) (ResourceInfo, error) {

	err := c.executeImpl(
		ctx,
		&state.State,
		func(ctx context.Context) error {
			return c.createImageFromURL(ctx, &state)
		},
		saveState,
		func(e error) bool {
			return isAlreadyCreatedError(e)
		},
	)
	if err != nil {
		return ResourceInfo{}, err
	}

	return c.CheckResourceReady(ctx, state.DstImageID)
}

func (c *client) CreateImageFromImage(
	ctx context.Context,
	state CreateImageFromImageState,
	saveState SaveStateFunc,
) (ResourceInfo, error) {

	err := c.executeImpl(
		ctx,
		&state.State,
		func(ctx context.Context) error {
			return c.createImageFromImage(ctx, &state)
		},
		saveState,
		func(e error) bool {
			return isAlreadyCreatedError(e)
		},
	)
	if err != nil {
		return ResourceInfo{}, err
	}

	return c.CheckResourceReady(ctx, state.DstImageID)
}

func (c *client) CreateImageFromSnapshot(
	ctx context.Context,
	state CreateImageFromSnapshotState,
	saveState SaveStateFunc,
) (ResourceInfo, error) {

	err := c.executeImpl(
		ctx,
		&state.State,
		func(ctx context.Context) error {
			return c.createImageFromSnapshot(ctx, &state)
		},
		saveState,
		func(e error) bool {
			return isAlreadyCreatedError(e)
		},
	)
	if err != nil {
		return ResourceInfo{}, err
	}

	return c.CheckResourceReady(ctx, state.DstImageID)
}

func (c *client) CreateImageFromDisk(
	ctx context.Context,
	state CreateImageFromDiskState,
	saveState SaveStateFunc,
) (ResourceInfo, error) {

	err := c.executeImpl(
		ctx,
		&state.State,
		func(ctx context.Context) error {
			return c.createImageFromDisk(ctx, &state)
		},
		saveState,
		func(e error) bool {
			return isAlreadyCreatedError(e)
		},
	)
	if err != nil {
		return ResourceInfo{}, err
	}

	return c.CheckResourceReady(ctx, state.DstImageID)
}

func (c *client) CreateSnapshotFromDisk(
	ctx context.Context,
	state CreateSnapshotFromDiskState,
	saveState SaveStateFunc,
) (ResourceInfo, error) {

	err := c.executeImpl(
		ctx,
		&state.State,
		func(ctx context.Context) error {
			return c.createSnapshotFromDisk(ctx, &state)
		},
		saveState,
		func(e error) bool {
			return isAlreadyCreatedError(e)
		},
	)
	if err != nil {
		return ResourceInfo{}, err
	}

	return c.CheckResourceReady(ctx, state.DstSnapshotID)
}

func (c *client) TransferFromImageToDisk(
	ctx context.Context,
	state TransferFromImageToDiskState,
	saveState SaveStateFunc,
) error {

	return c.execute(
		ctx,
		&state.State,
		func(ctx context.Context) error {
			return c.transferFromImageToDisk(ctx, &state)
		},
		saveState,
	)
}

func (c *client) TransferFromSnapshotToDisk(
	ctx context.Context,
	state TransferFromSnapshotToDiskState,
	saveState SaveStateFunc,
) error {

	return c.execute(
		ctx,
		&state.State,
		func(ctx context.Context) error {
			return c.transferFromSnapshotToDisk(ctx, &state)
		},
		saveState,
	)
}

func (c *client) DeleteImage(
	ctx context.Context,
	state DeleteImageState,
	saveState SaveStateFunc,
) error {

	return c.executeImpl(
		ctx,
		&state.State,
		func(ctx context.Context) error {
			return c.deleteImage(ctx, &state)
		},
		saveState,
		func(e error) bool {
			return isSnapshotNotFoundError(e)
		},
	)
}

func (c *client) DeleteSnapshot(
	ctx context.Context,
	state DeleteSnapshotState,
	saveState SaveStateFunc,
) error {

	return c.executeImpl(
		ctx,
		&state.State,
		func(ctx context.Context) error {
			return c.deleteSnapshot(ctx, &state)
		},
		saveState,
		func(e error) bool {
			return isSnapshotNotFoundError(e)
		},
	)
}

func (c *client) CheckResourceReady(
	ctx context.Context,
	resourceID string,
) (ResourceInfo, error) {

	reqCtx, err := c.getRequestContext(ctx)
	if err != nil {
		return ResourceInfo{}, err
	}

	snapshotInfo, err := c.snapshot.GetSnapshotInfo(reqCtx, resourceID)
	if err != nil {
		return ResourceInfo{}, wrapSnapshotInfoError(
			err,
			c.endpoints[c.endpointIdx],
		)
	}

	if snapshotInfo.State.Code != "ready" {
		e := fmt.Errorf(
			"unexpected status, resourceID=%v, status=%v",
			resourceID,
			snapshotInfo.State.Code,
		)

		// Yet another hack (NBS-2139).
		if snapshotInfo.State.Code == "creating" {
			return ResourceInfo{}, &errors.RetriableError{Err: e}
		}

		return ResourceInfo{}, &errors.NonRetriableError{Err: e}
	}

	return ResourceInfo{
		Size:        snapshotInfo.CreationInfo.Size,
		StorageSize: snapshotInfo.RealSize,
	}, nil
}

func (c *client) DeleteTask(ctx context.Context, taskID string) error {
	return c.snapshot.DeleteTask(ctx, taskID)
}
