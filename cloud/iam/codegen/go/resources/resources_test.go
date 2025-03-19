package resources

import (
	"testing"
)

func TestResources(t *testing.T) {
	if ResourceManagerCloud != "resource-manager.cloud" {
		t.Error("resources naming error")
	}
}
