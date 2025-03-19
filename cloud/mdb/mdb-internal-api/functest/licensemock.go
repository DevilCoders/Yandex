package functest

import (
	stub "a.yandex-team.ru/cloud/mdb/internal/compute/marketplace/stub"
)

func LicenseStub(allowCreateClusters bool) stub.LicenseMock {
	return stub.LicenseStub(allowCreateClusters)
}
