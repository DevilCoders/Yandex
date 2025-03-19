package compute

import (
	"context"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/compute/operations"
)

//go:generate ../../../scripts/mockgen.sh HostGroupService,ImageService,OperationService,HostTypeService

type Image struct {
	ID          string
	FolderID    string
	CreatedAt   time.Time
	Name        string
	Description string
	Labels      map[string]string
	Family      string
	StorageSize int64
	MinDiskSize int64
	ProductIDs  []string
	Status      string
	OS          OS
}

type ListImagesResponse struct {
	Images        []Image
	NextPageToken string
}

// All fields are optional
type ListImagesOpts struct {
	PageSize  int64
	PageToken string
	Filter    string
	Name      string
}

// All fields are optional
type CreateImageOpts struct {
	Description string
	Labels      map[string]string
	Family      string
	MinDiskSize int64
	ProductIds  []string
}

type OS string

const (
	OSLinux   OS = "LINUX"
	OSWindows OS = "WINDOWS"
)

type CreateImageSource interface {
	isCreateImageSource()
}

type CreateImageSourceImageID string
type CreateImageSourceDiskID string
type CreateImageSourceSnapshotID string
type CreateImageSourceURI string

func (u CreateImageSourceImageID) isCreateImageSource()    {}
func (u CreateImageSourceDiskID) isCreateImageSource()     {}
func (u CreateImageSourceSnapshotID) isCreateImageSource() {}
func (u CreateImageSourceURI) isCreateImageSource()        {}

type ListOperationsOpts struct {
	ImageID string
}

type ImageService interface {
	Get(ctx context.Context, imageID string) (Image, error)
	List(ctx context.Context, folderID string, opts ListImagesOpts) (ListImagesResponse, error)
	Delete(ctx context.Context, imageID string) (operations.Operation, error)
	Create(ctx context.Context, folderID, name string, source CreateImageSource, os OS, opts CreateImageOpts) (operations.Operation, error)
	UpdateLabels(ctx context.Context, imageID string, labels map[string]string) (operations.Operation, error)
	ListOperations(ctx context.Context, opts ListOperationsOpts) ([]operations.Operation, error)
}

type OperationService interface {
	Get(ctx context.Context, operationID string) (operations.Operation, error)
	GetDone(ctx context.Context, operationID string) (operations.Operation, error)
	Cancel(ctx context.Context, operationID string) (operations.Operation, error)
}

type DiskType string

const (
	DiskTypeNetworkHDD DiskType = "network-hdd"
	// if you need more types - add them
)

type Pool struct {
	Available []string `json:"available"`
	Creating  []string `json:"creating"`
	ImageID   string   `json:"imageId"`
	TypeID    string   `json:"typeId"`
	ZoneID    string   `json:"zoneId"`
}

type DiskPoolService interface {
	Create(ctx context.Context, iamToken, imageID string, typeID DiskType, size int) (operations.Operation, error)
	List(ctx context.Context, iamToken, imageID string) ([]Pool, error)
}

type HostGroup struct {
	ID                string
	FolderID          string
	CreatedAt         time.Time
	Name              string
	Description       string
	Labels            map[string]string
	ZoneID            string
	Status            string
	TypeID            string // points to HostType
	MaintenancePolicy string
}

// HostGroupService is a set of methods for managing groups of dedicated hosts
type HostGroupService interface {
	Get(ctx context.Context, hostGroupID string) (HostGroup, error)
}

type HostType struct {
	ID       string
	Cores    int64
	Memory   int64
	Disks    int64
	DiskSize int64
}

// HostTypeService - is a set of methods to view possible host configurations
type HostTypeService interface {
	Get(ctx context.Context, hostTypeID string) (HostType, error)
}

type HostGroupHostType struct {
	HostGroup HostGroup
	HostType  HostType
}

func (hght *HostGroupHostType) Generation() int64 {
	if strings.HasPrefix(hght.HostType.ID, "intel-6338") {
		return (int64)(3)
	}
	return (int64)(2)
}

func GetMinHostGroupResources(hostGroupHostType map[string]HostGroupHostType, zoneID string) (int64, int64, int64, int64, map[int64]string) {
	var minCores int64
	var minMemory int64
	diskSizeHostGroup := make(map[int64]string)
	var minDisks int64
	var diskSize int64
	for hostGroupID, v := range hostGroupHostType {
		if v.HostGroup.ZoneID == zoneID || zoneID == "" {
			if minCores == 0 {
				minCores = v.HostType.Cores
			}
			if minMemory == 0 {
				minMemory = v.HostType.Memory
			}
			if minDisks == 0 {
				minDisks = v.HostType.Disks
			}
			if v.HostType.Memory < minMemory {
				minMemory = v.HostType.Memory
			}
			if v.HostType.Cores < minCores {
				minCores = v.HostType.Cores
			}
			if v.HostType.Disks < minDisks {
				minDisks = v.HostType.Disks
			}
			diskSizeHostGroup[v.HostType.DiskSize] = hostGroupID
			diskSize = v.HostType.DiskSize
		}
	}
	return minCores, minMemory, minDisks, diskSize, diskSizeHostGroup
}
