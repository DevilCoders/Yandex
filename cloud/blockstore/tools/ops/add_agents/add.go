package main

import (
	"context"
	"crypto/md5"
	"encoding/hex"
	"errors"
	"fmt"
	"strconv"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/blockstore/config"
	"a.yandex-team.ru/cloud/blockstore/public/api/protos"
	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	"a.yandex-team.ru/cloud/blockstore/tools/common/go/cms"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/infra"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/juggler"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/pssh"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/release"
	st "a.yandex-team.ru/cloud/storage/core/tools/common/go/startrack"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/walle"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/z2"
)

////////////////////////////////////////////////////////////////////////////////

type ZoneConfig struct {
	Infra   *release.InfraConfig   `yaml:"infra"`
	Juggler *release.JugglerConfig `yaml:"juggler"`

	SchemaShardDir string  `yaml:"schemashard_dir"`
	ConfigHost     string  `yaml:"config_host"`
	CMSHost        string  `yaml:"cms_host"`
	KikimrHost     string  `yaml:"kikimr_host"`
	AllocationUnit uint64  `yaml:"allocation_unit"`
	LimitFraction  float64 `yaml:"limit_fraction"`
}

type ClusterConfig map[string]ZoneConfig

type ServiceConfig struct {
	Clusters map[string]ClusterConfig `yaml:"clusters"`
}

////////////////////////////////////////////////////////////////////////////////

type Options struct {
	Yes           bool
	DryRun        bool
	NoRestart     bool
	NoUpdateLimit bool
	SkipMutes     bool

	AllowNamedConfigs bool
	UseBlockDevices   bool
	EnableRDMA        bool

	ClusterName string
	ZoneName    string

	UserName string
	TempDir  string

	DeviceCount int
	Parallelism int

	RestartDelay     time.Duration
	RackRestartDelay time.Duration

	Cookie   string
	IAMToken string
	Ticket   string
}

////////////////////////////////////////////////////////////////////////////////

func updateDRConfig(
	dr *protos.TUpdateDiskRegistryConfigRequest,
	configs []*config.TDiskAgentConfig,
) (int, error) {
	var newAgents int

	knownAgents := make(map[string]*protos.TKnownDiskAgent)

	for _, agent := range dr.KnownAgents {
		knownAgents[agent.AgentId] = agent
	}

	for _, da := range configs {
		_, found := knownAgents[*da.AgentId]

		if found {
			continue
		}

		newAgents++

		agent := &protos.TKnownDiskAgent{
			AgentId: *da.AgentId,
		}

		for _, device := range da.FileDevices {
			agent.Devices = append(agent.Devices, *device.DeviceId)
		}

		for _, device := range da.MemoryDevices {
			agent.Devices = append(agent.Devices, *device.DeviceId)
		}

		for _, device := range da.NvmeDevices {
			agent.Devices = append(agent.Devices, device.DeviceIds...)
		}

		dr.KnownAgents = append(dr.KnownAgents, agent)
	}

	return newAgents, nil
}

func makeAgentConfig(
	host string,
	deviceCount int,
	useBlockDevices bool,
	enableRDMA bool,
) *config.TDiskAgentConfig {

	var devices []*config.TFileDeviceArgs

	pathFmt := "/dev/disk/by-partlabel/NVMENBS%02d"
	if useBlockDevices {
		pathFmt = "/dev/nvme3n%v"
	}

	for i := 1; i != deviceCount+1; i++ {
		h := md5.New()
		fmt.Fprintf(h, "%s-%02d", host, i)
		deviceID := hex.EncodeToString(h.Sum(nil))

		path := fmt.Sprintf(pathFmt, i)
		blockSize := uint32(4096)

		devices = append(
			devices,
			&config.TFileDeviceArgs{
				Path:      &path,
				BlockSize: &blockSize,
				DeviceId:  &deviceID,
			},
		)
	}

	enabled := true
	backend := config.EDiskAgentBackendType_DISK_AGENT_BACKEND_AIO
	nqn := "nqn.2018-09.io.spdk:cnode1"
	nvmeTarget := config.TNVMeTargetArgs{
		Nqn: &nqn,
	}
	dedicated := true
	seqno := uint64(1)

	var rdmaTarget *config.TRdmaTarget

	if enableRDMA {
		rdmaTarget = &config.TRdmaTarget{
			Endpoint: &protos.TRdmaEndpoint{
				Host: host,
				Port: 10020,
			},
			Server: &config.TRdmaServer{
				Backlog:          64,
				QueueSize:        512,
				MaxBufferSize:    4198400, // 4MB data + 4KB meta
				KeepAliveTimeout: 100,
				WaitMode:         config.EWaitMode_WAIT_MODE_ADAPTIVE_WAIT,
				PollerThreads:    1,
			},
		}
	}

	return &config.TDiskAgentConfig{
		AgentId:            &host,
		Enabled:            &enabled,
		Backend:            &backend,
		NvmeTarget:         &nvmeTarget,
		FileDevices:        devices,
		DedicatedDiskAgent: &dedicated,
		SeqNumber:          &seqno,
		RdmaTarget:         rdmaTarget,
	}
}

func generateConfigs(
	hosts []string,
	deviceCount int,
	useBlockDevices bool,
	enableRDMA bool,
) []*config.TDiskAgentConfig {

	var configs []*config.TDiskAgentConfig

	for _, host := range hosts {
		configs = append(
			configs,
			makeAgentConfig(
				strings.TrimSpace(host),
				deviceCount,
				useBlockDevices,
				enableRDMA,
			),
		)
	}

	return configs
}

func calcLimit(
	dr *protos.TUpdateDiskRegistryConfigRequest,
	allocationUnit uint64,
	p float64,
) uint64 {

	var total uint64

	for _, agent := range dr.KnownAgents {
		total += uint64(len(agent.Devices)) * allocationUnit
	}

	return uint64(float64(total) * p)
}

func updateNRDLimit(
	ctx context.Context,
	limit uint64,
	kikimr kikimrClientIface,
) error {

	err := kikimr.SetUserAttribute(
		ctx,
		"__volume_space_limit_ssd_nonrepl",
		strconv.FormatUint(limit, 10),
	)
	if err != nil {
		return err
	}

	value, err := kikimr.GetUserAttribute(ctx, "__volume_space_limit_ssd_nonrepl")
	if err != nil {
		return err
	}

	current, err := strconv.ParseUint(value, 10, 64)
	if err != nil {
		return fmt.Errorf("can't get current NRD limit. Bad value: %v", current)
	}

	if current != limit {
		return fmt.Errorf("can't set NRD limit: %v != %v", current, limit)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

type z2FakeClient struct {
	logutil.WithLog

	workers []string
}

func (client *z2FakeClient) EditItems(
	ctx context.Context,
	configID string,
	items []z2.Z2Item,
) error {
	return errors.New("EditItems not implemented")
}

func (client *z2FakeClient) Update(
	ctx context.Context,
	configID string,
	forceYes bool,
) error {
	return errors.New("Update not implemented")
}

func (client *z2FakeClient) UpdateStatus(
	ctx context.Context,
	configID string,
) (*z2.Z2UpdateStatus, error) {
	return nil, errors.New("UpdateStatus not implemented")
}

func (client *z2FakeClient) ListWorkers(
	ctx context.Context,
	configID string,
) ([]string, error) {
	client.LogInfo(ctx, "[Z2] ListWorkers: %v", configID)

	return client.workers, nil
}

////////////////////////////////////////////////////////////////////////////////

func applyDAConfigs(
	ctx context.Context,
	agents []*config.TDiskAgentConfig,
	cookie string,
	cms cms.CMSClientIface,
) error {
	var errors []error

	for i := range agents {
		config := agents[i]
		err := cms.UpdateConfig(ctx, *config.AgentId, config, cookie)
		if err != nil {
			errors = append(
				errors,
				fmt.Errorf(
					"can't apply configs to '%v': %w",
					*config.AgentId,
					err,
				),
			)
		}
	}

	if len(errors) == 0 {
		return nil
	}

	return fmt.Errorf("%v", errors)
}

////////////////////////////////////////////////////////////////////////////////

func addAgents(
	ctx context.Context,
	log nbs.Log,
	opts Options,
	service *ServiceConfig,
	hosts []string,
	infra infra.InfraClientIface,
	juggler juggler.JugglerClientIface,
	walle walle.WalleClientIface,
	pssh pssh.PsshIface,
	st st.StarTrackClientIface,
) (
	*protos.TUpdateDiskRegistryConfigRequest,
	[]*config.TDiskAgentConfig,
	error,
) {

	if len(hosts) == 0 {
		return nil, nil, errors.New("nothing to add")
	}

	cluster, found := service.Clusters[opts.ClusterName]
	if !found {
		return nil, nil, fmt.Errorf("unknown cluster name '%v'", opts.ClusterName)
	}

	zone, found := cluster[opts.ZoneName]
	if !found {
		return nil, nil, fmt.Errorf(
			"unknown zone name '%v' for cluster '%v'",
			opts.ZoneName,
			opts.ClusterName,
		)
	}

	agents := generateConfigs(
		hosts,
		opts.DeviceCount,
		opts.UseBlockDevices,
		opts.EnableRDMA,
	)

	nbs := newNBS(
		log,
		zone.ConfigHost,
		opts.UserName,
		opts.TempDir,
		opts.IAMToken,
		pssh,
	)

	dr, err := nbs.DescribeDiskRegistryConfig(ctx)
	if err != nil {
		return nil, nil, fmt.Errorf("can't load DR config: %w", err)
	}

	newAgents, err := updateDRConfig(dr, agents)
	if err != nil {
		return nil, nil, fmt.Errorf("failed to update DR config: %w", err)
	}

	if opts.DryRun {
		return dr, agents, nil
	}

	if newAgents != 0 {
		err = nbs.UpdateDiskRegistryConfig(ctx, dr)
		if err != nil {
			return nil, nil, fmt.Errorf("can't update DR config: %w", err)
		}
	}

	cmsClient := cms.NewClient(
		log,
		zone.CMSHost,
		opts.UserName,
		opts.TempDir,
		pssh,
	)

	if opts.AllowNamedConfigs {
		err := cmsClient.AllowNamedConfigs(ctx)
		if err != nil {
			return nil, nil, fmt.Errorf("can't allow named configs: %w", err)
		}
	}

	err = applyDAConfigs(
		ctx,
		agents,
		opts.Cookie,
		cmsClient,
	)

	if err != nil {
		return nil, nil, fmt.Errorf("can't apply DA configs: %w", err)
	}

	if !opts.NoUpdateLimit {
		err = updateNRDLimit(
			ctx,
			calcLimit(
				dr,
				zone.AllocationUnit,
				zone.LimitFraction,
			),
			newKikimrClient(
				log,
				zone.KikimrHost,
				zone.SchemaShardDir,
				pssh,
			),
		)
		if err != nil {
			return nil, nil, fmt.Errorf("can't update NRD limits: %w", err)
		}
	}

	if opts.NoRestart {
		return dr, agents, nil
	}

	r := release.NewRelease(
		log,
		&release.Options{
			Yes:               opts.Yes,
			SkipInfraAndMutes: opts.SkipMutes,
			ClusterName:       opts.ClusterName,
			ZoneName:          opts.ZoneName,
			Description:       "add new disk agents",
			Ticket:            opts.Ticket,
			Parallelism:       opts.Parallelism,
			RackRestartDelay:  opts.RackRestartDelay,
		},
		&release.ServiceConfig{
			Name:        "disk_agent",
			Description: "DiskAgent",
			Clusters: map[string]release.ClusterConfig{
				opts.ClusterName: map[string]release.ZoneConfig{
					opts.ZoneName: release.ZoneConfig{
						Infra:   zone.Infra,
						Juggler: zone.Juggler,
						Targets: []*release.TargetConfig{
							&release.TargetConfig{
								Name: "nbs",
								Services: []release.UnitConfig{
									release.UnitConfig{
										Name:         "blockstore-disk-agent",
										RestartDelay: release.Duration(opts.RestartDelay),
									},
								},
								Z2: &release.Z2Config{
									Group:     "DISK_AGENT_GROUP",
									MetaGroup: "DISK_AGENT_GROUP",
								},
							},
						},
					},
				},
			},
		},
		infra,
		juggler,
		pssh,
		st,
		nil, // tg
		walle,
		&z2FakeClient{
			WithLog: logutil.WithLog{Log: log},
			workers: hosts,
		},
	)

	err = r.Run(ctx)

	return dr, agents, err
}
