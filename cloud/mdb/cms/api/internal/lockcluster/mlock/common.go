package mlock

import (
	"fmt"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/clusterdiscovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
)

type Locker struct {
	client    mlockclient.Locker
	otherLegs clusterdiscovery.OtherLegsDiscovery
}

func NewLocker(client mlockclient.Locker, otherLegs clusterdiscovery.OtherLegsDiscovery) lockcluster.Locker {
	return &Locker{
		client:    client,
		otherLegs: otherLegs,
	}
}

func genLockID(taskID string, clusterID string) string {
	return fmt.Sprintf("cms-%s-%s", taskID, clusterID)
}
