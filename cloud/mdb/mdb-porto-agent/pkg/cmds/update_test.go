package cmds

import (
	"context"
	"fmt"
	"path"
	"strconv"
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/require"

	mocks_dns "a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/dns/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/porto"
	mocks_papi "a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/porto-api/mocks"
	mocks_porto "a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/porto/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/statestore"
	mocks_ss "a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/statestore/mocks"
	portoapi "a.yandex-team.ru/infra/tcp-sampler/pkg/porto"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/test/yatest"
)

const (
	projQuotaScript = "/usr/bin/project_quota"
	bootstrapScript = "/usr/local/yandex/porto/mdb_super.sh"

	testResolveWaitPeriod = time.Microsecond
)

var (
	errResolve = xerrors.Errorf("resolve")
)

func getLogger() log.Logger {
	logger, _ := zap.New(zap.KVConfig(log.DebugLevel))
	return logger
}

func createCommon(t *testing.T, container string, dryRun bool) (updateCtx, *gomock.Controller, secrets, *porto.Support, *mocks_papi.API, *mocks_porto.MockRunner, *mocks_porto.MockNetwork, *mocks_ss.Storage, *mocks_dns.MockResolver) {
	uctx := updateCtx{
		log:       getLogger(),
		container: container,
		drm:       dryRun,
	}

	ctrl := gomock.NewController(t)

	ss := new(mocks_ss.Storage)
	net := mocks_porto.NewMockNetwork(ctrl)
	pr := mocks_porto.NewMockRunner(ctrl)
	api := new(mocks_papi.API)
	ps, err := porto.New(uctx.log, uctx.drm, api, net)
	require.NoError(t, err)
	resolver := mocks_dns.NewMockResolver(ctrl)

	var secrets secrets
	return uctx, ctrl, secrets, ps, api, pr, net, ss, resolver
}

func hasContainerAndVolumes(api *mocks_papi.API, container string, volPaths []string) []portoapi.TVolumeDescription {
	api.On("List1", container).Once().Return([]string{container}, nil)
	var vds []portoapi.TVolumeDescription
	for i, vp := range volPaths {
		vd := portoapi.TVolumeDescription{
			Path: fmt.Sprintf("/place/porto_volumes/%d/volume", i+1),
			Properties: map[string]string{
				"storage": vp,
			},
			Containers: []string{container},
		}
		vds = append(vds, vd)
	}
	api.On("ListVolumes", "", "").Return(vds, nil)
	return vds
}

func hasContainerAndVolumesDetailed(t *testing.T, api *mocks_papi.API, container string, volPaths []string, volSize []uint64) []portoapi.TVolumeDescription {
	require.Equal(t, len(volPaths), len(volSize))
	api.On("List1", container).Once().Return([]string{container}, nil)
	var vds []portoapi.TVolumeDescription
	for i, vp := range volPaths {
		vd := portoapi.TVolumeDescription{
			Path: fmt.Sprintf("/place/porto_volumes/%d/volume", i+1),
			Properties: map[string]string{
				"storage":         vp,
				"space_limit":     strconv.FormatUint(volSize[i], 10),
				"backend":         "native",
				"group":           "root",
				"inode_guarantee": "0",
				"inode_limit":     "0",
				"permissions":     "0775",
				"space_guarantee": "0",
				"user":            "root",
			},
			Containers: []string{container},
		}
		vds = append(vds, vd)
	}
	api.On("ListVolumes", "", "").Return(vds, nil)
	api.On("GetProperty", container, "state").Return("running", nil)
	api.On("GetProperty", container, "root").Return(vds[0].Path, nil)
	api.On("GetProperty", container, "hostname").Return(container, nil)
	api.On("GetProperty", container, "virt_mode").Return("os", nil)
	api.On("GetProperty", container, "cwd").Return("/", nil)
	api.On("GetProperty", container, "sysctl").Return("", nil)
	api.On("GetProperty", container, "net").Return("L3 eth0", nil)
	return vds
}

func TestUpdateDryRunCreateContainer(t *testing.T) {
	container := "test-dry-create-container.db.yandex.net"
	ctx := context.Background()
	uctx, _, secrets, ps, api, pr, _, ss, resolver := createCommon(t, container, true)

	// no real create container because of dry run
	api.On("List1", container).Return([]string{}, nil)
	api.On("ListVolumes", "", "").Return([]portoapi.TVolumeDescription{}, nil)
	co := statestore.ContainerOptions{
		CPUGuarantee: "1c",
		BootstrapCmd: bootstrapScript,
	}
	rootPath := path.Join("tmp_write_notexpected", container, "rootfs")
	vo := statestore.VolumeOptions{
		Dom0Path:   rootPath,
		Path:       "/",
		SpaceLimit: 10 << 30,
	}
	st := statestore.State{
		Options: co,
		Volumes: []statestore.VolumeOptions{vo},
	}
	changes, err := updateContainer(ctx, uctx, ps, pr, ss, secrets, st, "", resolver, resolveWaitTimeout, testResolveWaitPeriod)
	require.NoError(t, err)
	require.Equal(t, 6, len(changes))
	require.Equal(t, fmt.Sprintf("create porto container %s", container), changes[0])
	require.Equal(t, fmt.Sprintf("create directory %s", rootPath), changes[1])
	require.Equal(t, fmt.Sprintf("create volume %s", rootPath), changes[2])
	require.Equal(t, fmt.Sprintf("check project quota for path %s, volume: /some/new/volume/path", rootPath), changes[3])
	require.Equal(t, fmt.Sprintf("bootstrap script '%s %s'", bootstrapScript, container), changes[4])
	require.Equal(t, fmt.Sprintf("run container %s, because it's in state not exist", container), changes[5])
	api.AssertExpectations(t)
}

func TestUpdateDryRunDeleteContainer(t *testing.T) {
	container := "test-dry-delete-container.db.yandex.net"
	ctx := context.Background()
	uctx, _, secrets, ps, api, pr, _, ss, resolver := createCommon(t, container, true)

	testPath := yatest.OutputPath(uuid.Must(uuid.NewV4()).String())
	rootPath := path.Join(testPath, container, "rootfs")
	dataPath := path.Join(testPath, container, "datafs")
	hasContainerAndVolumes(api, container, []string{rootPath, dataPath})

	// no real delete container because of dry run
	co := statestore.ContainerOptions{
		CPUGuarantee:  "1c",
		MemLimit:      4 << 30,
		PendingDelete: true,
	}
	vor := statestore.VolumeOptions{
		Dom0Path:   path.Join("tmp_write_notexpected", container, "rootfs"),
		Path:       "/",
		SpaceLimit: 10 << 30,
	}
	vod := statestore.VolumeOptions{
		Dom0Path:   path.Join("tmp_write_notexpected", container, "datafs"),
		Path:       "/",
		SpaceLimit: 16 << 30,
	}
	st := statestore.State{
		Options: co,
		Volumes: []statestore.VolumeOptions{vor, vod},
	}
	changes, err := updateContainer(ctx, uctx, ps, pr, ss, secrets, st, "", resolver, resolveWaitTimeout, testResolveWaitPeriod)
	require.NoError(t, err)
	require.Equal(t, 4, len(changes))
	api.AssertExpectations(t)
}

func TestUpdateCreateContainer(t *testing.T) {
	container := "test-update-create-container.db.yandex.net"
	ctx := context.Background()
	uctx, _, secrets, ps, api, pr, net, ss, resolver := createCommon(t, container, false)

	useVlan688 := false
	projID := "1234:4321"
	managingProjID := "6789:9876"
	ipAddr := "eth0 2a02:6b8:0:51e:1234:4321:8c34:6e8f"
	api.On("List1", container).Once().Return([]string{}, nil)
	api.On("ListVolumes", "", "").Once().Return([]portoapi.TVolumeDescription{}, nil)
	api.On("Create", container).Once().Return(nil)
	api.On("CreateVolume", "", mock.AnythingOfType("map[string]string")).Once().Return(portoapi.TVolumeDescription{}, nil)
	api.On("GetProperty", container, "ip").Return("", nil)
	api.On("GetProperty", container, mock.AnythingOfType("string")).Return(mock.Anything, nil)
	api.On("SetProperty", container, "hostname", container).Once().Return(nil)
	api.On("SetProperty", container, "ip", ipAddr).Once().Return(nil)
	api.On("SetProperty", container, "thread_limit", "32000").Once().Return(nil)
	api.On("SetProperty", container, mock.AnythingOfType("string"), mock.AnythingOfType("string")).Return(nil)
	api.On("Start", container).Return(nil)
	pr.EXPECT().RunCommandOnDom0(projQuotaScript, "check", gomock.AssignableToTypeOf("")).Return(nil)
	pr.EXPECT().RunCommandOnDom0(bootstrapScript, container).Return(nil)
	net.EXPECT().GetExpectedIPAddrs(container, projID, managingProjID, useVlan688).Return(ipAddr, nil)
	resolveRetryCount := 3
	pr.EXPECT().RunCommandOnPortoContainer(container, "/usr/bin/sudo", "-u", "selfdns", "/usr/bin/selfdns-client", "--debug").Return(nil).Times(resolveRetryCount)
	resolver.EXPECT().LookupHost(container).Return(nil, errResolve).Times(resolveRetryCount)
	resolver.EXPECT().LookupHost(container).Return(nil, nil)

	co := statestore.ContainerOptions{
		CPUGuarantee:      "1c",
		UseVLAN688:        useVlan688,
		ProjectID:         projID,
		ManagingProjectID: managingProjID,
		BootstrapCmd:      bootstrapScript,
	}
	rndPath := uuid.Must(uuid.NewV4()).String()
	testPath := yatest.OutputPath(uuid.Must(uuid.NewV4()).String())
	rootPath := path.Join(testPath, rndPath, container, "rootfs")
	vo := statestore.VolumeOptions{
		Dom0Path:   rootPath,
		Path:       "/",
		SpaceLimit: 10 << 30,
	}
	st := statestore.State{
		Options: co,
		Volumes: []statestore.VolumeOptions{vo},
	}
	changes, err := updateContainer(ctx, uctx, ps, pr, ss, secrets, st, "", resolver, resolveWaitTimeout, testResolveWaitPeriod)
	require.NoError(t, err)
	require.Equal(t, 28, len(changes))
	api.AssertExpectations(t)
}

func TestPropertyWithStopContainer(t *testing.T) {
	container := "test-update-property-with-stop-container.db.yandex.net"
	ctx := context.Background()
	uctx, _, secrets, ps, api, pr, net, ss, resolver := createCommon(t, container, false)

	useVlan688 := false
	projID := "1234:4321"
	managingProjID := "6789:9876"
	oldThreadLimit := "10000"
	newThreadLimit := "32000"

	testPath := yatest.OutputPath(uuid.Must(uuid.NewV4()).String())
	rootPath := path.Join(testPath, container, "rootfs")
	dataPath := path.Join(testPath, container, "datafs")

	rootSize := uint64(8) << 30
	dataSize := uint64(16) << 30
	vor := statestore.VolumeOptions{
		Dom0Path:   rootPath,
		Path:       "/",
		SpaceLimit: rootSize,
	}
	dataMountPoint := "/var/log/mysql"
	vod := statestore.VolumeOptions{
		Dom0Path:   dataPath,
		Path:       dataMountPoint,
		SpaceLimit: dataSize,
	}
	vds := hasContainerAndVolumesDetailed(t, api, container, []string{rootPath, dataPath}, []uint64{rootSize, dataSize})
	for _, vs := range vds {
		pr.EXPECT().RunCommandOnDom0(projQuotaScript, "check", vs.Path).Return(nil)
	}
	ipAddrOld := "eth0 2a02:6b8:0:51e:dead:beef:8c34:6e8f"
	ipAddrNew := "eth0 2a02:6b8:0:51e:1234:4321:8c34:6e8f"
	for opt, value := range map[string]string{
		"ip":               ipAddrOld,
		"bind":             fmt.Sprintf("%s %s rw", dataPath, dataMountPoint),
		"project_id":       projID,
		"thread_limit":     oldThreadLimit,
		"bootstrap_cmd":    bootstrapScript,
		"cpu_guarantee":    "",
		"cpu_limit":        "",
		"sysctl":           "",
		"resolv_conf":      "",
		"delete_token":     "",
		"net_guarantee":    "",
		"net_rx_limit":     "",
		"net_limit":        "",
		"capabilities":     "container-capabilities",
		"core_command":     "container-core-command",
		"pending_delete":   "false",
		"oom_is_fatal":     "false",
		"use_vlan688":      "false",
		"io_limit":         "0",
		"memory_guarantee": "0",
		"memory_limit":     "0",
		"anon_limit":       "0",
		"hugetlb_limit":    "0",
	} {
		api.On("GetProperty", container, opt).Return(value, nil)
	}

	api.On("SetProperty", container, "hostname", container).Return(nil)
	api.On("Stop", container).Return(nil)
	api.On("SetProperty", container, "ip", ipAddrNew).Once().Return(nil)
	api.On("SetProperty", container, "capabilities", "container-capabilities").Return(nil)
	api.On("SetProperty", container, "thread_limit", newThreadLimit).Return(nil)
	api.On("SetProperty", container, "core_command", "container-core-command").Return(nil)
	api.On("Start", container).Return(nil)
	pr.EXPECT().RunCommandOnDom0(bootstrapScript, container).Return(nil)
	net.EXPECT().GetExpectedIPAddrs(container, projID, managingProjID, useVlan688).Return(ipAddrNew, nil)
	pr.EXPECT().RunCommandOnPortoContainer(container, "/usr/bin/sudo", "-u", "selfdns", "/usr/bin/selfdns-client", "--debug").Return(nil)
	resolver.EXPECT().LookupHost(container).Return(nil, errResolve)
	resolver.EXPECT().LookupHost(container).Return(nil, nil)
	api.On("GetProperty", container, mock.AnythingOfType("string")).Return("some_string", nil)

	st := statestore.State{
		Options: statestore.ContainerOptions{
			ProjectID:         projID,
			ManagingProjectID: managingProjID,
			BootstrapCmd:      bootstrapScript,
			Capabilities:      "container-capabilities",
			CoreCommand:       "container-core-command",
		},
		Volumes: []statestore.VolumeOptions{vor, vod},
	}
	changes, err := updateContainer(ctx, uctx, ps, pr, ss, secrets, st, "", resolver, resolveWaitTimeout, testResolveWaitPeriod)
	expectedChanges := []string{
		fmt.Sprintf("create directory %s", rootPath),
		fmt.Sprintf("check project quota for path %s, volume: %s", rootPath, vds[0].Path),
		fmt.Sprintf("create directory %s", dataPath),
		fmt.Sprintf("check project quota for path %s, volume: %s", dataPath, vds[1].Path),
		fmt.Sprintf("stop container %s, because it's in state running", container),
		fmt.Sprintf("change property ip of container %s (%s -> %s)", container, ipAddrOld, ipAddrNew),
		fmt.Sprintf("change property thread_limit of container %s (10000 -> 32000)", container),
		// bootstrap, because no emulate root fs totaly
		fmt.Sprintf("bootstrap script '%s %s'", bootstrapScript, container),
		fmt.Sprintf("run container %s, because it's in state stopped", container),
	}
	require.NoError(t, err)
	require.Equal(t, expectedChanges, changes)
}

func TestUpdateDeleteState(t *testing.T) {
	container := "test-update-delete-state.db.yandex.net"
	ctx := context.Background()
	uctx, _, secrets, ps, api, pr, _, ss, resolver := createCommon(t, container, false)

	api.On("List1", container).Once().Return([]string{}, nil)
	ss.On("RemoveState", container).Once().Return(nil)
	co := statestore.ContainerOptions{
		CPUGuarantee:  "1c",
		MemLimit:      4 << 30,
		PendingDelete: true,
	}
	vor := statestore.VolumeOptions{
		Dom0Path:   path.Join("tmp_write_notexpected", container, "rootfs"),
		Path:       "/",
		SpaceLimit: 10 << 30,
	}
	vod := statestore.VolumeOptions{
		Dom0Path:   path.Join("tmp_write_notexpected", container, "datafs"),
		Path:       "/",
		SpaceLimit: 16 << 30,
	}
	st := statestore.State{
		Options: co,
		Volumes: []statestore.VolumeOptions{vor, vod},
	}
	changes, err := updateContainer(ctx, uctx, ps, pr, ss, secrets, st, "", resolver, resolveWaitTimeout, testResolveWaitPeriod)
	require.NoError(t, err)
	require.Equal(t, 1, len(changes))
	require.Equal(t, fmt.Sprintf("remove state for container %s", container), changes[0])
	api.AssertExpectations(t)
	ss.AssertExpectations(t)
}

func TestUpdateDestroyContainerAndVolume(t *testing.T) {
	container := "test-update-destroy-container-and-volume.db.yandex.net"
	ctx := context.Background()
	uctx, _, secrets, ps, api, pr, _, ss, resolver := createCommon(t, container, false)

	testPath := yatest.OutputPath(uuid.Must(uuid.NewV4()).String())
	rootPath := path.Join(testPath, container, "rootfs")
	dataPath := path.Join(testPath, container, "datafs")
	vds := hasContainerAndVolumes(api, container, []string{rootPath, dataPath})

	api.On("UnlinkVolume", vds[0].Path, container).Once().Return(nil)
	api.On("UnlinkVolume", vds[1].Path, container).Once().Return(nil)
	api.On("Destroy", container).Once().Return(nil)
	ss.On("RemoveState", container).Once().Return(nil)
	co := statestore.ContainerOptions{
		CPUGuarantee:  "1c",
		MemLimit:      4 << 30,
		PendingDelete: true,
	}
	vor := statestore.VolumeOptions{
		Dom0Path:   path.Join(testPath, container, "rootfs"),
		Path:       "/",
		SpaceLimit: 10 << 30,
	}
	vod := statestore.VolumeOptions{
		Dom0Path:   path.Join(testPath, container, "datafs"),
		Path:       "/var/log/mysql",
		SpaceLimit: 16 << 30,
	}
	st := statestore.State{
		Options: co,
		Volumes: []statestore.VolumeOptions{vor, vod},
	}
	changes, err := updateContainer(ctx, uctx, ps, pr, ss, secrets, st, "", resolver, resolveWaitTimeout, testResolveWaitPeriod)
	require.NoError(t, err)
	require.Equal(t, 4, len(changes))
	require.Equal(t, fmt.Sprintf("unlink volume %s", rootPath), changes[0])
	require.Equal(t, fmt.Sprintf("unlink volume %s", dataPath), changes[1])
	require.Equal(t, fmt.Sprintf("destroy porto container %s", container), changes[2])
	require.Equal(t, fmt.Sprintf("remove state for container %s", container), changes[3])
	api.AssertExpectations(t)
	ss.AssertExpectations(t)
}
