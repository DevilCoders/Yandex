package godogutil

import "a.yandex-team.ru/library/go/test/yatest"

func TestOutputPath(relativePath string) string {
	if !yatest.HasYaTestContext() {
		return relativePath
	} else if yatest.HasRAMDrive() {
		return yatest.OutputRAMDrivePath(relativePath)
	} else {
		return yatest.OutputPath(relativePath)
	}
}
