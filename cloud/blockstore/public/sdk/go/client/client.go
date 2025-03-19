package client

import (
	"golang.org/x/net/context"

	protos "a.yandex-team.ru/cloud/blockstore/public/api/protos"
)

////////////////////////////////////////////////////////////////////////////////

type Client struct {
	safeClient
}

func (client *Client) MountVolume(
	ctx context.Context,
	diskId string,
	opts *MountVolumeOpts,
) (*protos.TVolume, *SessionInfo, error) {

	req := &protos.TMountVolumeRequest{
		DiskId: diskId,
	}

	if opts != nil {
		req.Token = opts.Token
		req.VolumeAccessMode = opts.AccessMode
		req.VolumeMountMode = opts.MountMode
		req.MountFlags = opts.MountFlags
	}

	resp, err := client.Impl.MountVolume(ctx, req)
	if err != nil {
		return nil, nil, err
	}

	timeout := durationFromMsec(int64(resp.GetInactiveClientsTimeout()))

	session := &SessionInfo{
		SessionId:              resp.GetSessionId(),
		InactiveClientsTimeout: timeout,
	}

	return resp.GetVolume(), session, nil
}

func (client *Client) UnmountVolume(
	ctx context.Context,
	diskId string,
	sessionId string,
) error {
	req := &protos.TUnmountVolumeRequest{
		DiskId:    diskId,
		SessionId: sessionId,
	}

	_, err := client.Impl.UnmountVolume(ctx, req)
	return err
}

func (client *Client) ReadBlocks(
	ctx context.Context,
	diskId string,
	startIndex uint64,
	blocksCount uint32,
	checkpointId string,
	sessionId string,
) ([][]byte, error) {

	req := &protos.TReadBlocksRequest{
		DiskId:       diskId,
		StartIndex:   startIndex,
		BlocksCount:  blocksCount,
		CheckpointId: checkpointId,
		SessionId:    sessionId,
	}

	resp, err := client.Impl.ReadBlocks(ctx, req)
	if err != nil {
		return nil, err
	}

	return resp.Blocks.Buffers, nil
}

func (client *Client) WriteBlocks(
	ctx context.Context,
	diskId string,
	startIndex uint64,
	blocks [][]byte,
	sessionId string,
) error {
	req := &protos.TWriteBlocksRequest{
		DiskId:     diskId,
		StartIndex: startIndex,
		Blocks: &protos.TIOVector{
			Buffers: blocks,
		},
		SessionId: sessionId,
	}

	_, err := client.Impl.WriteBlocks(ctx, req)
	return err
}

func (client *Client) ZeroBlocks(
	ctx context.Context,
	diskId string,
	startIndex uint64,
	blocksCount uint32,
	sessionId string,
) error {
	req := &protos.TZeroBlocksRequest{
		DiskId:      diskId,
		StartIndex:  startIndex,
		BlocksCount: blocksCount,
		SessionId:   sessionId,
	}

	_, err := client.Impl.ZeroBlocks(ctx, req)
	return err
}

func (client *Client) StartEndpoint(
	ctx context.Context,
	unixSocketPath string,
	diskId string,
	ipcType protos.EClientIpcType,
	clientId string,
	instanceId string,
	accessMode protos.EVolumeAccessMode,
	mountMode protos.EVolumeMountMode,
	mountSeqNumber uint64,
	vhostQueuesCount uint32,
	unalignedRequestsDisabled bool,
) (*protos.TVolume, error) {
	req := &protos.TStartEndpointRequest{
		UnixSocketPath:            unixSocketPath,
		DiskId:                    diskId,
		IpcType:                   ipcType,
		ClientId:                  clientId,
		InstanceId:                instanceId,
		VolumeAccessMode:          accessMode,
		VolumeMountMode:           mountMode,
		MountSeqNumber:            mountSeqNumber,
		VhostQueuesCount:          vhostQueuesCount,
		UnalignedRequestsDisabled: unalignedRequestsDisabled,
	}

	resp, err := client.Impl.StartEndpoint(ctx, req)
	if err != nil {
		return nil, err
	}

	return resp.GetVolume(), err
}

func (client *Client) StopEndpoint(
	ctx context.Context,
	unixSocketPath string,
) error {
	req := &protos.TStopEndpointRequest{
		UnixSocketPath: unixSocketPath,
	}

	_, err := client.Impl.StopEndpoint(ctx, req)
	return err
}

func (client *Client) ListEndpoints(
	ctx context.Context,
) ([]*protos.TStartEndpointRequest, error) {
	req := &protos.TListEndpointsRequest{}

	resp, err := client.Impl.ListEndpoints(ctx, req)
	if err != nil {
		return nil, err
	}

	return resp.GetEndpoints(), nil
}

func (client *Client) KickEndpoint(
	ctx context.Context,
	keyringId uint32,
) error {
	req := &protos.TKickEndpointRequest{
		KeyringId: keyringId,
	}

	_, err := client.Impl.KickEndpoint(ctx, req)
	return err
}

func (client *Client) ListKeyrings(
	ctx context.Context,
) ([]*protos.TListKeyringsResponse_TKeyringEndpoint, error) {
	req := &protos.TListKeyringsRequest{}

	resp, err := client.Impl.ListKeyrings(ctx, req)
	if err != nil {
		return nil, err
	}

	return resp.GetEndpoints(), nil
}

func (client *Client) DescribeEndpoint(
	ctx context.Context,
	unixSocketPath string,
) (*protos.TClientPerformanceProfile, error) {
	req := &protos.TDescribeEndpointRequest{
		UnixSocketPath: unixSocketPath,
	}

	resp, err := client.Impl.DescribeEndpoint(ctx, req)
	if err != nil {
		return nil, err
	}

	return resp.GetPerformanceProfile(), nil
}

func (client *Client) QueryAvailableStorage(
	ctx context.Context,
	agentIds []string,
) ([]*protos.TAvailableStorageInfo, error) {

	req := &protos.TQueryAvailableStorageRequest{
		AgentIds: agentIds,
	}

	resp, err := client.Impl.QueryAvailableStorage(ctx, req)
	if err != nil {
		return nil, err
	}

	return resp.GetAvailableStorage(), nil
}

func (client *Client) CmsRemoveHost(
	ctx context.Context,
	host string,
	dryRun bool,
) (*protos.TCmsActionResponse, error) {

	t := protos.TAction_REMOVE_HOST
	req := &protos.TCmsActionRequest{
		Actions: []*protos.TAction{{
			Type:   &t,
			Host:   &host,
			DryRun: &dryRun,
		}},
	}

	resp, err := client.Impl.CmsAction(ctx, req)
	if err != nil {
		return nil, err
	}

	return resp, nil
}

func (client *Client) CmsGetDependentDisks(
	ctx context.Context,
	host string,
) ([]string, error) {

	t := protos.TAction_GET_DEPENDENT_DISKS
	req := &protos.TCmsActionRequest{
		Actions: []*protos.TAction{{
			Type: &t,
			Host: &host,
		}},
	}

	resp, err := client.Impl.CmsAction(ctx, req)
	if err != nil {
		return nil, err
	}

	res := resp.GetActionResults()
	if len(res) == 0 {
		return []string{}, nil
	}

	return res[0].GetDependentDisks(), nil
}

////////////////////////////////////////////////////////////////////////////////

func NewClient(
	grpcOpts *GrpcClientOpts,
	durableOpts *DurableClientOpts,
	log Log,
) (*Client, error) {

	grpcClient, err := NewGrpcClient(grpcOpts, log)
	if err != nil {
		return nil, err
	}

	durableClient := NewDurableClient(grpcClient, durableOpts, log)

	return &Client{
		safeClient{durableClient},
	}, nil
}
