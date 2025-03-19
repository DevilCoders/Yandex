package mysyncutil

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"sort"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type MysyncInfo struct {
	raw map[string]interface{}
}

type DiskState struct {
	Used  uint64
	Total uint64
}

func ReadMysyncInfoFile(mysyncInfoFilePath string) (MysyncInfo, error) {
	jsonFile, err := os.Open(mysyncInfoFilePath)
	if err != nil {
		return MysyncInfo{}, err
	}
	defer jsonFile.Close()

	byteValue, _ := ioutil.ReadAll(jsonFile)
	jsonMap := make(map[string]interface{})
	err = json.Unmarshal(byteValue, &jsonMap)
	if err != nil {
		return MysyncInfo{}, err
	}

	return MysyncInfo{raw: jsonMap}, nil
}

func readMysyncInfoString(jsonData string) (MysyncInfo, error) {
	jsonMap := make(map[string]interface{})
	err := json.Unmarshal([]byte(jsonData), &jsonMap)
	if err != nil {
		return MysyncInfo{}, err
	}

	return MysyncInfo{raw: jsonMap}, nil
}

func (mi *MysyncInfo) Maintenance() bool {
	// check /maintenance
	_, ok := mi.raw["maintenance"]
	return ok
}

func (mi *MysyncInfo) GetMaster() string {
	hostname, ok := mi.raw["master"].(string)
	if !ok {
		return ""
	}
	return hostname
}

func (mi *MysyncInfo) IsReadOnly(hostname string) (bool, error) {
	// check health -> hostname -> is_readonly
	health, ok := mi.raw["health"].(map[string]interface{})
	if !ok {
		return false, xerrors.New("Cannot parse JSON: no object found at '$.health' json-path")
	}
	hostData, ok := health[hostname].(map[string]interface{})
	if !ok {
		return false, xerrors.New(fmt.Sprintf("Cannot parse JSON: no object found at '$.health.%s' json-path", hostname))
	}
	isReadOnly, ok := hostData["is_readonly"].(bool)
	if !ok {
		return false, xerrors.New(fmt.Sprintf("Cannot parse JSON: no boolean found at '$.health.%s.is_readonly' json-path", hostname))
	}
	return isReadOnly, nil
}

func (mi *MysyncInfo) DiskState(hostname string) (DiskState, error) {
	// check health -> hostname -> disk_state
	health, ok := mi.raw["health"].(map[string]interface{})
	if !ok {
		return DiskState{}, xerrors.New("Cannot parse JSON: no object found at '$.health' json-path")
	}
	hostData, ok := health[hostname].(map[string]interface{})
	if !ok {
		return DiskState{}, xerrors.New(fmt.Sprintf("Cannot parse JSON: no object found at '$.health.%s' json-path", hostname))
	}
	diskState, ok := hostData["disk_state"].(map[string]interface{})
	if !ok {
		return DiskState{}, xerrors.New(fmt.Sprintf("Cannot parse JSON: no object found at '$.health.%s.disk_state' json-path", hostname))
	}
	return DiskState{
		Used:  uint64(diskState["Used"].(float64)),
		Total: uint64(diskState["Total"].(float64)),
	}, nil
}

func (mi *MysyncInfo) HANodes() ([]string, error) {
	// check ha_nodes
	nodes, ok := mi.raw["ha_nodes"].(map[string]interface{})
	if !ok {
		if _, ok = mi.raw["ha_nodes"]; ok {
			return []string{}, nil
		} else {
			return []string{}, xerrors.New("Cannot parse JSON: no object found at '$.ha_nodes' json-path")
		}
	}
	var result []string
	for host := range nodes {
		result = append(result, host)
	}
	sort.Strings(result)
	return result, nil
}

func (mi *MysyncInfo) CascadeNodes() (map[string]string, error) {
	// check cascade_nodes -> hostname -> StreamFrom
	nodes, ok := mi.raw["cascade_nodes"].(map[string]interface{})
	if !ok {
		if _, ok = mi.raw["cascade_nodes"]; ok {
			return map[string]string{}, nil
		} else {
			return map[string]string{}, xerrors.New("Cannot parse JSON: no object found at '$.cascade_nodes' json-path")
		}
	}
	result := make(map[string]string)
	for host, rawCNC := range nodes {
		cnc, ok := rawCNC.(map[string]interface{})
		if !ok {
			return result, xerrors.New(fmt.Sprintf("Cannot parse JSON: no object found at '$.health.%s' json-path", host))
		}
		streamFrom, ok := cnc["stream_from"].(string)
		if !ok {
			return result, xerrors.New(fmt.Sprintf("Cannot parse JSON: no string found at '$.health.%s.stream_from' json-path", host))
		}
		result[host] = streamFrom
	}
	return result, nil
}

// Returns hosts that is expected to fetch binlogs directly from `hostname`
func (mi *MysyncInfo) ExpectedReplicasOf(hostname string) ([]string, error) {
	result := make([]string, 0)
	cascadeNodes, err := mi.CascadeNodes()
	if err != nil {
		return result, err
	}
	haNodes, err := mi.HANodes()
	if err != nil {
		return result, err
	}

	for host, streamFrom := range cascadeNodes {
		if hostname == streamFrom {
			result = append(result, host)
		}
	}

	// all HA nodes (except master) should replicate from master
	if hostname == mi.GetMaster() {
		for _, node := range haNodes {
			if node != hostname {
				result = append(result, node)
			}
		}
	}

	return result, nil
}
