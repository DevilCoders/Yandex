package statestore

import (
	"time"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// Errors
var (
	ErrEmtpyContainerArgument  = xerrors.NewSentinel("empty container argument")
	ErrMissedContainerState    = xerrors.NewSentinel("missed or unreadable container state")
	ErrInvalidContainerState   = xerrors.NewSentinel("invalid container state")
	ErrInvalidContainerExState = xerrors.NewSentinel("invalid container extra state")
	ErrStateNotExist           = xerrors.NewSentinel("state not exist")
)

const (
	// DatetimeTemplate of file timestamp
	DatetimeTemplate = "2006-01-02T15:04:05"
)

type Storage interface {
	// ListContainers get all available contaner states in cache
	ListContainers() ([]string, error)

	// GetTargetState load target state, save history and check, that it's not locked
	GetTargetState(target string) (container string, state State, err error)

	// CleanHistory remove obsolete history states of container
	CleanHistory(dryRun bool, container string, ttl time.Duration)

	// RemoveState for container
	RemoveState(container string) error
}

// ContainerOptions is base container options
type ContainerOptions struct {
	BootstrapCmd      string `json:"bootstrap_cmd"`
	CPUGuarantee      string `json:"cpu_guarantee"`
	CPULimit          string `json:"cpu_limit"`
	CoreCommand       string `json:"core_command"`
	IOLimit           uint64 `json:"io_limit,string"`
	MemGuarantee      uint64 `json:"memory_guarantee,string"`
	MemLimit          uint64 `json:"memory_limit,string"`
	AnonLimit         uint64 `json:"anon_limit,string"`
	HugeTLBLimit      uint64 `json:"hugetlb_limit,string"`
	NetGuarantee      string `json:"net_guarantee"`
	NetRxLimit        string `json:"net_rx_limit"`
	NetLimit          string `json:"net_limit"`
	ThreadLimit       string `json:"thread_limit"`
	IP                string `json:"ip"`
	SysCtl            string `json:"sysctl"`
	ResolvConf        string `json:"resolv_conf"`
	ProjectID         string `json:"project_id"`
	ManagingProjectID string `json:"managing_project_id"`
	PendingDelete     bool   `json:"pending_delete"`
	DeleteToken       string `json:"delete_token,omitempty"`
	OOMIsFatal        bool   `json:"oom_is_fatal"`
	Capabilities      string `json:"capabilities"`
	UseVLAN688        bool   `json:"use_vlan688"`
}

// VolumeOptions is volume options for container
type VolumeOptions struct {
	Backend        string `json:"backend"`
	Dom0Path       string `json:"dom0_path"`
	Path           string `json:"path"`
	PendingBackup  bool   `json:"pending_backup"`
	ReadOnly       bool   `json:"read_only"`
	User           string `json:"user,omitempty"`
	Group          string `json:"group,omitempty"`
	Permissions    string `json:"permissions,omitempty"`
	SpaceGuarantee uint64 `json:"space_guarantee,omitempty"`
	SpaceLimit     uint64 `json:"space_limit,omitempty"`
	InodeGuarantee uint64 `json:"inode_guarantee,omitempty"`
	InodeLimit     uint64 `json:"inode_limit,omitempty"`
}

// State used state in container
type State struct {
	Options ContainerOptions `json:"container_options"`
	Volumes []VolumeOptions  `json:"volumes"`
}
