package permissions

import (
	"testing"
)

func TestPermissions(t *testing.T) {
	if IamServiceAccountsUse != "iam.serviceAccounts.use" {
		t.Error("permissions naming error")
	}
}
