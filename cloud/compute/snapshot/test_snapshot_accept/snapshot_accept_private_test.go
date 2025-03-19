package test_snapshot_accept

import (
	"testing"

	grpcCompute "bb.yandex-team.ru/cloud/cloud-go/genproto/privateapi/yandex/cloud/priv/compute/v1"
	"bb.yandex-team.ru/cloud/cloud-go/genproto/publicapi/yandex/cloud/compute/v1"
	"bb.yandex-team.ru/cloud/compute/go/common/pkg/constants"
	. "bb.yandex-team.ru/cloud/compute/go/tests/fixtures"
)

const (
	defaultDiskSize = constants.GB
)

func TestCreateDiskFromImage(t *testing.T) {
	env := NewEnv(t, nil)
	for _, zone := range Zones(env) {
		zoneID := zone // fix value - for parallel run
		t.Run(zone, func(t *testing.T) {
			env := NewEnv(t, nil)
			c := ComputeDiskClient(env)
			defaultImage := DefaultImage(env)
			op := WaitOperationMust(env)(c.Create(env.Ctx(), &grpcCompute.CreateDiskRequest{
				FolderId: DefaultFolderID(env),
				Labels:   Label(env, nil),
				ZoneId:   zoneID,
				Size:     defaultDiskSize,
				Source:   &grpcCompute.CreateDiskRequest_ImageId{ImageId: defaultImage.Id},
			}))
			var disk compute.Disk
			MustUnmarshalResponse(env, &op, &disk)
			env.R().Equal(zoneID, disk.ZoneId)
			env.R().Equal(int64(defaultDiskSize), disk.Size)
			env.R().Equal(defaultImage.Id, disk.GetSourceImageId())
		})
	}
}

func TestCreateImageFromURL(t *testing.T) {
	table := []struct {
		name, url string
	}{
		{"direct", "https://storage.yandexcloud.net/snapshot-test/test-images/compressed-small-cirros-0.3.5-x86_64-disk.img"},
	}

	for i := range table {
		item := table[i]
		t.Run(item.name, func(t *testing.T) {
			env := NewEnv(t, nil)
			c := ComputeImageClient(env)
			WaitOperationMust(env)(c.Create(env.Ctx(), &grpcCompute.CreateImageRequest{
				FolderId: DefaultFolderID(env),
				Labels:   Label(env, nil),
				Source:   &grpcCompute.CreateImageRequest_Uri{Uri: item.url},
			}))
		})
	}
}

func TestCreateImageFromImage(t *testing.T) {
	env := NewEnv(t, nil)
	c := ComputeImageClient(env)
	WaitOperationMust(env)(c.Create(env.Ctx(), &grpcCompute.CreateImageRequest{
		FolderId: DefaultFolderID(env),
		Labels:   Label(env, nil),
		Source:   &grpcCompute.CreateImageRequest_ImageId{ImageId: DefaultImage(env).Id},
	}))
}

func TestCreateSnapshotFromDisk(t *testing.T) {
	env := NewEnv(t, nil)
	for _, zone := range Zones(env) {
		zoneID := zone
		t.Run(zone, func(t *testing.T) {
			env := NewEnv(t, nil)
			defaultImage, disk := NewDiskWithData(env, zoneID)
			c := ComputeDiskClient(env)
			env.T().Logf("Disk: %v - %v", disk.Id, disk.Status)
			defer func() {
				disk2, err := c.Get(env.Ctx(), &grpcCompute.GetDiskRequest{DiskId: disk.Id})
				env.T().Logf("Disk2: %v - %v (%v)", disk2.Id, disk2.Status, err)
			}()
			snapshotClient := ComputeSnapshotClient(env)
			op := WaitOperationMust(env)(snapshotClient.Create(env.Ctx(), &grpcCompute.CreateSnapshotRequest{
				FolderId: DefaultFolderID(env),
				DiskId:   disk.Id,
				Labels:   Label(env, nil),
			}))

			var snapshot compute.Snapshot
			MustUnmarshalResponse(env, &op, &snapshot)
			env.R().Equal(defaultImage.StorageSize, snapshot.StorageSize)
			disk.GetTypeId()
		})
	}
}
