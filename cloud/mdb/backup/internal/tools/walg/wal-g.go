package walg

import (
	"fmt"
	"path"
	"regexp"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/math"
)

const (
	WalgSentinelSuffix        = "_backup_stop_sentinel.json"
	WalgBaseBackupsFolderName = "basebackups_005"
	WalgWALsFolderName        = "wal_005"
)

var (
	WalgSentinelRegexp          = regexp.MustCompile(`^(?P<root>.*)\/basebackups_005\/(?P<name>.*)_backup_stop_sentinel\.json`)
	WalgSentinelRegexpPathIndex = WalgSentinelRegexp.SubexpIndex("root")
	WalgSentinelRegexpNameIndex = WalgSentinelRegexp.SubexpIndex("name")
	WalgSentinelRegexpMaxIndex  = math.MaxInt(WalgSentinelRegexpPathIndex, WalgSentinelRegexpNameIndex)
)

func ParseWalgSentinelPath(path string) (rootPath string, backupName string, err error) {
	res := WalgSentinelRegexp.FindStringSubmatch(path)
	if len(res) != WalgSentinelRegexpMaxIndex+1 {
		return "", "", xerrors.Errorf("can not extract rootPath from sentinel path %+v: %q", res, path)
	}
	return res[WalgSentinelRegexpPathIndex], res[WalgSentinelRegexpNameIndex], nil
}

func WalgBackupsPathFromRootPath(rootPath string) (backupsPath string) {
	return fmt.Sprintf("%s/", path.Join(rootPath, WalgBaseBackupsFolderName))
}

func WalgWalPathFromRootPath(rootPath string) (walsPath string) { // TODO: rename - remove Walg prefix
	return fmt.Sprintf("%s/", path.Join(rootPath, WalgWALsFolderName))
}
