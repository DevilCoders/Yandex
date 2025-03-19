package quotas

import (
	"testing"
)

func TestQuotas(t *testing.T) {
	if ComputeDisksCount != "compute.disks.count" {
		t.Error("quotas naming error")
	}
}
