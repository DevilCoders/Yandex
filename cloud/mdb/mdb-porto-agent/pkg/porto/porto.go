package porto

import (
	"fmt"
	"os"
	"strconv"
	"strings"

	"github.com/c2h5oh/datasize"

	"a.yandex-team.ru/cloud/mdb/internal/portoutil/properties"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/statestore"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/supp"
	portoapi "a.yandex-team.ru/infra/tcp-sampler/pkg/porto"
	portorpc "a.yandex-team.ru/infra/tcp-sampler/pkg/rpc"
	l "a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Errors
var (
	ErrPortoAPIUnexpected = xerrors.NewSentinel("porto API unexpected bahaviour")
	ErrPortoAPIFailed     = xerrors.NewSentinel("porto API failed")

	ErrNoExpectedInterfaceAddr = xerrors.NewSentinel("no expected interface address on dom0 host")
)

const (
	rootAcc = "root"
)

//go:generate ../../../scripts/mockgen.sh Network,Runner

// Runner allow direct run command on porto container
type Runner interface {
	RunCommandOnPortoContainer(container, name string, args ...string) error
	RunCommandOnDom0(name string, args ...string) error
}

// Network support IP address hints for container
type Network interface {
	GetExpectedIPAddrs(container, projID, managingProjID string, useVLAN688 bool) (string, error)
}

// Support porto support struct
type Support struct {
	log l.Logger
	api portoapi.API
	drm bool // dry run mode
	net Network
}

// Volume description struct
type Volume struct {
	Path           string
	Backend        string
	User           string
	Group          string
	DirMode        uint64
	InodeGuarantee uint64
	InodeLimit     uint64
	SpaceGuarantee uint64
	SpaceLimit     uint64
}

// New create new
func New(log l.Logger, dryRun bool, api portoapi.API, net Network) (*Support, error) {
	return &Support{
		log: log,
		api: api,
		drm: dryRun,
		net: net,
	}, nil
}

// EnsureAbsent container not exist and volumes not linked
func (ps *Support) EnsureAbsent(changes []string, container string) ([]string, error) {
	ls, err := ps.api.List1(container)
	if err != nil {
		return changes, ErrPortoAPIFailed.Wrap(err)
	}
	bf := []l.Field{l.String("module", "porto"), l.String("func", "EnsureAbsent"), l.String("container", container)}

	if len(ls) == 0 {
		ps.log.Debug("container not found", bf...)
		return changes, nil
	}
	if len(ls) > 1 || ls[0] != container {
		ps.log.Errorf("unexpected api call result, container by name return %v, for container %s", ls, container)
		return changes, ErrPortoAPIUnexpected.WithFrame()
	}

	volumes, err := ps.api.ListVolumes("", "")
	if err != nil {
		return changes, ErrPortoAPIFailed.Wrap(err)
	}

	for _, vol := range volumes {
		if !hasContainer(vol.Containers, container) {
			continue
		}
		dom0path := vol.Properties["storage"]
		msg := fmt.Sprintf("unlink volume %s", dom0path)
		changes, err = supp.DoAction(ps.log, ps.drm, changes, msg, func() error {
			if err = ps.api.UnlinkVolume(vol.Path, container); err != nil {
				if ps.api.GetLastError() == portorpc.EError_ContainerDoesNotExist {
					ps.log.Infof("container %s already not exist, unlink volume at EnsureAbsent do nothing", container)
					return nil
				}
				return ErrPortoAPIFailed.Wrap(err)
			}
			return nil
		})
		if err != nil {
			return changes, err
		}
	}

	msg := fmt.Sprintf("destroy porto container %s", container)
	return supp.DoAction(ps.log, ps.drm, changes, msg, func() error {
		if err = ps.api.Destroy(container); err != nil {
			if ps.api.GetLastError() == portorpc.EError_ContainerDoesNotExist {
				ps.log.Infof("container %s already not exist, destroy container at EnsureAbsent do nothing", container)
				return nil
			}
			return ErrPortoAPIFailed.Wrap(err)
		}
		return nil
	})
}

// EnsureContainer check and create container if necessary
func (ps *Support) EnsureContainer(changes []string, container string) ([]string, error) {
	ls, err := ps.api.List1(container)
	if err != nil {
		return changes, ErrPortoAPIFailed.Wrap(err)
	}
	for _, ci := range ls {
		if ci == container {
			return changes, nil
		}
	}

	msg := fmt.Sprintf("create porto container %s", container)
	return supp.DoAction(ps.log, ps.drm, changes, msg, func() error {
		if err = ps.api.Create(container); err != nil {
			if ps.api.GetLastError() == portorpc.EError_ContainerAlreadyExists {
				ps.log.Infof("container %s already exist, create container at EnsureContainer do nothing", container)
				return nil
			}
			return ErrPortoAPIFailed.Wrap(err)
		}
		return nil
	})
}

// EnsureVolumes make volume with specified properties as target
func (ps *Support) EnsureVolumes(changes []string, container string, vos []statestore.VolumeOptions, pr Runner) ([]string, map[string]string, error) {
	// TODO: list for concrete porto container return only first one; fix code, when will used porto version without this bug
	//volumes, err := ps.api.ListVolumes("", container)
	volumes, err := ps.api.ListVolumes("", "")
	if err != nil {
		return changes, nil, ErrPortoAPIFailed.Wrap(err)
	}

	for _, vol := range volumes {
		if !hasContainer(vol.Containers, container) {
			continue
		}
		dom0path := vol.Properties["storage"]
		ps.log.Debugf("container have volume %s", dom0path)
		if hasDom0Path(dom0path, vos) {
			continue
		}
		msg := fmt.Sprintf("unlink volume %s", dom0path)
		changes, err = supp.DoAction(ps.log, ps.drm, changes, msg, func() error {
			if err = ps.api.UnlinkVolume(vol.Path, container); err != nil {
				return ErrPortoAPIFailed.Wrap(err)
			}
			return nil
		})
		if err != nil {
			return changes, nil, err
		}
	}

	volByDom0 := make(map[string]string, len(vos))
	for _, vo := range vos {
		changeLen := len(changes)
		changes, err = ps.ensurePortoVolume(changes, volumes, container, volByDom0, vo)
		if len(changes) != changeLen {
			msg := fmt.Sprintf("check project quota for path %s, volume: %s", vo.Dom0Path, volByDom0[vo.Dom0Path])
			changes, err = supp.DoAction(ps.log, ps.drm, changes, msg, func() error {
				return pr.RunCommandOnDom0("/usr/bin/project_quota", "check", volByDom0[vo.Dom0Path])
			})
		}
		if err != nil {
			return changes, volByDom0, err
		}
	}

	return changes, volByDom0, nil
}

type containerContext struct {
	container string
	isStopped bool
	changes   []string
	err       error
}

// EnsureContainerProperties check that container has correct properties
func (ps *Support) EnsureContainerProperties(changes []string, container string, st statestore.State, volByDom0 map[string]string, checkRunning bool) ([]string, bool, error) {
	var rootPath string
	var bindVolumes []string
	for _, vo := range st.Volumes {
		if vo.Path == "/" {
			rootPath = vo.Dom0Path
		} else {
			vt := "rw"
			if vo.ReadOnly {
				vt = "ro"
			}
			l := []string{vo.Dom0Path, vo.Path, vt}
			bindVolumes = append(bindVolumes, strings.Join(l, " "))
		}
	}
	if st.Options.AnonLimit == 0 {
		st.Options.AnonLimit = uint64(float64(st.Options.MemLimit) * 0.95)
	}
	if st.Options.Capabilities == "" {
		st.Options.Capabilities = "CHOWN;DAC_OVERRIDE;FOWNER;FSETID;KILL;SETGID;SETUID;SETPCAP;LINUX_IMMUTABLE;NET_BIND_SERVICE;" +
			"NET_ADMIN;NET_RAW;IPC_LOCK;SYS_CHROOT;SYS_PTRACE;SYS_BOOT;SYS_NICE;SYS_RESOURCE;MKNOD;AUDIT_WRITE;SETFCAP"
	}
	if st.Options.CoreCommand == "" {
		st.Options.CoreCommand = "cp --sparse=always /dev/stdin /var/cores/${CORE_EXE_NAME}-${CORE_PID}-S${CORE_SIG}.core"
	}
	if st.Options.ThreadLimit == "" {
		st.Options.ThreadLimit = "32000"
	}

	cCtx := containerContext{
		container: container,
		isStopped: !checkRunning,
		changes:   changes,
	}

	if st.Options.IP == "" {
		ip, err := ps.net.GetExpectedIPAddrs(container, st.Options.ProjectID, st.Options.ManagingProjectID, st.Options.UseVLAN688)
		if err != nil {
			return cCtx.changes, cCtx.isStopped, err
		}
		st.Options.IP = ip
	}

	cCtx = ps.ensureContainerProperty(cCtx, "hostname", container)
	cCtx = ps.ensureContainerProperty(cCtx, "ip", st.Options.IP)
	cCtx = ps.ensureContainerProperty(cCtx, "root", volByDom0[rootPath])
	cCtx = ps.ensureContainerProperty(cCtx, "bind", strings.Join(bindVolumes, ";"))
	cCtx = ps.ensureContainerProperty(cCtx, "virt_mode", "os")
	cCtx = ps.ensureContainerProperty(cCtx, "cwd", "/")
	cCtx = ps.ensureContainerProperty(cCtx, "sysctl", st.Options.SysCtl)
	cCtx = ps.ensureContainerProperty(cCtx, "net", "L3 eth0")
	cCtx = ps.ensureContainerProperty(cCtx, "cpu_guarantee", st.Options.CPUGuarantee)
	cCtx = ps.ensureContainerProperty(cCtx, "cpu_limit", st.Options.CPULimit)
	cCtx = ps.ensureContainerProperty(cCtx, "io_limit", strconv.FormatUint(st.Options.IOLimit, 10))
	cCtx = ps.ensureContainerProperty(cCtx, "memory_guarantee", strconv.FormatUint(st.Options.MemGuarantee, 10))
	cCtx = ps.ensureContainerProperty(cCtx, "memory_limit", strconv.FormatUint(st.Options.MemLimit, 10))
	cCtx = ps.ensureContainerProperty(cCtx, "anon_limit", strconv.FormatUint(st.Options.AnonLimit, 10))
	cCtx = ps.ensureContainerProperty(cCtx, "hugetlb_limit", strconv.FormatUint(st.Options.HugeTLBLimit, 10))
	cCtx = ps.ensureContainerProperty(cCtx, "net_guarantee", st.Options.NetGuarantee)
	cCtx = ps.ensureContainerProperty(cCtx, "net_rx_limit", st.Options.NetRxLimit)
	cCtx = ps.ensureContainerProperty(cCtx, "net_limit", st.Options.NetLimit)
	cCtx = ps.ensureContainerProperty(cCtx, "oom_is_fatal", strconv.FormatBool(st.Options.OOMIsFatal))
	cCtx = ps.ensureContainerProperty(cCtx, "capabilities", st.Options.Capabilities)
	cCtx = ps.ensureContainerProperty(cCtx, "core_command", st.Options.CoreCommand)
	cCtx = ps.ensureContainerProperty(cCtx, "thread_limit", st.Options.ThreadLimit)
	if st.Options.ResolvConf != "" {
		cCtx = ps.ensureContainerProperty(cCtx, "resolv_conf", st.Options.ResolvConf)
	}
	return cCtx.changes, cCtx.isStopped, cCtx.err
}

// EnsureRunning check that container is running
func (ps *Support) EnsureRunning(changes []string, container string, isContainer, isStopped bool) ([]string, error) {
	state := "not exist"
	if isContainer && isStopped {
		state = "stopped"
	}
	if isContainer && !isStopped {
		state, err := ps.api.GetProperty(container, "state")
		if err != nil {
			return changes, ErrPortoAPIFailed.Wrap(err)
		}
		if state == "running" {
			return changes, nil
		}
	}
	msg := fmt.Sprintf("run container %s, because it's in state %s", container, state)
	return supp.DoAction(ps.log, ps.drm, changes, msg, func() error {
		if err := ps.api.Start(container); err != nil {
			return ErrPortoAPIFailed.Wrap(err)
		}
		return nil
	})
}

// EnsureStopped check that container is stopped
func (ps *Support) ensureStopped(changes []string, container string) ([]string, error) {
	state, err := ps.api.GetProperty(container, "state")
	if err != nil {
		return changes, ErrPortoAPIFailed.Wrap(err)
	}
	if state == "stopped" {
		return changes, nil
	}
	msg := fmt.Sprintf("stop container %s, because it's in state %s", container, state)
	return supp.DoAction(ps.log, ps.drm, changes, msg, func() error {
		if err := ps.api.Stop(container); err != nil {
			return ErrPortoAPIFailed.Wrap(err)
		}
		return nil
	})
}

func hasDom0Path(dom0path string, vos []statestore.VolumeOptions) bool {
	for _, vo := range vos {
		if dom0path == vo.Dom0Path {
			return true
		}
	}
	return false
}

func (ps *Support) ensurePortoVolume(changes []string, volumes []portoapi.TVolumeDescription, container string, volByDom0 map[string]string, vo statestore.VolumeOptions) ([]string, error) {
	props := make(map[string]string, 10)
	if vo.Backend == "" {
		vo.Backend = "native"
	}
	props["backend"] = vo.Backend
	if vo.User == "" {
		vo.User = rootAcc
	}
	props["user"] = vo.User
	if vo.Group == "" {
		vo.Group = rootAcc
	}
	props["group"] = vo.Group
	if vo.Permissions == "" {
		vo.Permissions = "0775"
	}
	props["permissions"] = vo.Permissions
	props["inode_guarantee"] = strconv.FormatUint(vo.InodeGuarantee, 10)
	props["inode_limit"] = strconv.FormatUint(vo.InodeLimit, 10)
	props["space_guarantee"] = strconv.FormatUint(vo.SpaceGuarantee, 10)
	props["space_limit"] = strconv.FormatUint(vo.SpaceLimit, 10)

	if _, err := os.Stat(vo.Dom0Path); os.IsNotExist(err) {
		msg := fmt.Sprintf("create directory %s", vo.Dom0Path)
		changes, err = supp.DoAction(ps.log, ps.drm, changes, msg, func() error {
			return os.MkdirAll(vo.Dom0Path, os.ModeDir|0755)
		})
		if err != nil {
			return changes, err
		}
	}

	for _, vol := range volumes {
		if vol.Properties["storage"] != vo.Dom0Path {
			continue
		}
		volByDom0[vo.Dom0Path] = vol.Path
		vdChg, propChg := changedProperties(vol.Properties, props)
		if len(vdChg) == 0 {
			return changes, nil
		}
		msg := fmt.Sprintf("tune volume %s, changes: [%s]", vo.Dom0Path, strings.Join(vdChg, "; "))
		changes, err := supp.DoAction(ps.log, ps.drm, changes, msg, func() error {
			if err := ps.api.TuneVolume(vol.Path, propChg); err != nil {
				return ErrPortoAPIFailed.Wrap(err)
			}
			return nil
		})
		if err != nil {
			return changes, err
		}
		if hasContainer(vol.Containers, container) {
			return changes, nil
		}
		return ps.linkVolume(changes, container, vo.Dom0Path, volByDom0)
	}

	msg := fmt.Sprintf("create volume %s", vo.Dom0Path)
	props["storage"] = vo.Dom0Path
	props["containers"] = container
	volByDom0[vo.Dom0Path] = "/some/new/volume/path"
	return supp.DoAction(ps.log, ps.drm, changes, msg, func() error {
		ps.log.Infof("creating volume path %s, %+v", vo.Path, props)
		cvd, err := ps.api.CreateVolume("", props)
		if err != nil {
			return ErrPortoAPIFailed.Wrap(err)
		}
		volByDom0[vo.Dom0Path] = cvd.Path
		return nil
	})
}

func prettyContainerProperty(property, curVal, newVal string) (string, string) {
	isNet := strings.HasPrefix(property, "net_")
	if isNet || strings.HasPrefix(property, "memory_") || strings.HasPrefix(property, "anon_") || strings.HasPrefix(property, "io_") {
		if isNet {
			curVal = strings.TrimPrefix(curVal, "default: ")
			newVal = strings.TrimPrefix(newVal, "default: ")
		}
		if curVal != "" {
			cvu, err := strconv.ParseUint(curVal, 10, 0)
			if err == nil {
				curVal = datasize.ByteSize(cvu).HR()
			}
		}
		vu, err := strconv.ParseUint(newVal, 10, 0)
		if err == nil {
			newVal = datasize.ByteSize(vu).HR()
		}
	}
	if curVal == "" {
		curVal = "<None>"
	}
	return curVal, newVal
}

func (ps *Support) ensureContainerProperty(cCtx containerContext, property, value string) containerContext {
	if cCtx.err != nil {
		return cCtx
	}
	var curVal string
	curVal, cCtx.err = ps.api.GetProperty(cCtx.container, property)
	if cCtx.err != nil {
		return cCtx
	}
	if ok, err := propertyIsEqual(property, curVal, value); err != nil {
		cCtx.err = xerrors.Errorf("check property equal: %w", err)
		return cCtx
	} else if ok {
		return cCtx
	}
	curVal, pVal := prettyContainerProperty(property, curVal, value)

	if !cCtx.isStopped {
		switch property {
		case "bind", "root":
			cCtx.err = xerrors.Errorf("not same RO property %s for container %s, '%s' while expect '%s'", property, cCtx.container, curVal, pVal)
			return cCtx
		case "ip", "capabilities", "sysctl", "cwd", "virt_mode", "net", "hostname":
			{
				cCtx.changes, cCtx.err = ps.ensureStopped(cCtx.changes, cCtx.container)
				if cCtx.err != nil {
					return cCtx
				}
				cCtx.isStopped = true
			}
		}
	}

	msg := fmt.Sprintf("change property %s of container %s (%s -> %s)", property, cCtx.container, curVal, pVal)
	cCtx.changes, cCtx.err = supp.DoAction(ps.log, ps.drm, cCtx.changes, msg, func() error {
		if err := ps.api.SetProperty(cCtx.container, property, value); err != nil {
			return ErrPortoAPIFailed.Wrap(xerrors.Errorf("failed %s, error: %+v", msg, err))
		}
		return nil
	})
	return cCtx
}

func (ps *Support) linkVolume(changes []string, container, dom0Path string, volByDom0 map[string]string) ([]string, error) {
	msg := fmt.Sprintf("link volume %s", dom0Path)
	return supp.DoAction(ps.log, ps.drm, changes, msg, func() error {
		if err := ps.api.LinkVolume(volByDom0[dom0Path], container); err != nil {
			return ErrPortoAPIFailed.Wrap(err)
		}
		return nil
	})
}

func hasContainer(containers []string, target string) bool {
	for _, c := range containers {
		if c == target {
			return true
		}
	}
	return false
}

func changedProperties(current, goal map[string]string) ([]string, map[string]string) {
	var changes []string
	propChg := make(map[string]string)
	for prop, gv := range goal {
		cv, ok := current[prop]
		if ok && cv == gv {
			continue
		}
		pcv := cv
		pgv := gv
		if strings.HasPrefix(prop, "space") {
			if ok {
				scv, err := strconv.ParseUint(cv, 10, 0)
				if err == nil {
					pcv = datasize.ByteSize(scv).HR()
				}
			} else {
				pcv = "<None>"
			}
			sgv, err := strconv.ParseUint(gv, 10, 0)
			if err == nil {
				pgv = datasize.ByteSize(sgv).HR()
			}
		}
		changes = append(changes, fmt.Sprintf("%s (%s -> %s)", prop, pcv, pgv))
		propChg[prop] = gv
	}
	return changes, propChg
}

func propertyIsEqual(property, curVal, value string) (bool, error) {
	switch property {
	case "ip":
		if curVal == "" {
			return false, nil
		}

		curIPs, err := properties.ParseIPProperty(curVal)
		if err != nil {
			return false, xerrors.Errorf("parse current IP property: %w", err)
		}

		ips, err := properties.ParseIPProperty(value)
		if err != nil {
			return false, xerrors.Errorf("parse new IP property: %w", err)
		}

		if len(curIPs) != len(ips) {
			return false, nil
		}
		for i, ip := range ips {
			if ip != curIPs[i] {
				return false, nil
			}
		}
		return true, nil
	}
	return curVal == value, nil
}
