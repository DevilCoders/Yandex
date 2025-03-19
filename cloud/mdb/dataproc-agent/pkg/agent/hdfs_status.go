package agent

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"
	"strings"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type nodesInfo struct {
	LiveNodesRaw string `json:"LiveNodes"`
	DeadNodesRaw string `json:"DeadNodes"`
}

type hdfsRespNodes struct {
	Beans []nodesInfo `json:"beans"`
}

type hdfsResp struct {
	Beans []models.HDFSInfo `json:"beans"`
}

func removePort(fqdn string) string {
	idx := strings.LastIndex(fqdn, ":")
	if idx == -1 {
		return fqdn
	}
	return fqdn[0:idx]
}

func parseNodes(info models.HDFSInfo, nodes nodesInfo) (models.HDFSInfo, error) {
	var err error
	nodesMap := make(map[string]models.HDFSNodeInfo)
	data := []byte(nodes.LiveNodesRaw)
	if err = json.Unmarshal(data, &nodesMap); err != nil {
		return models.HDFSInfo{}, xerrors.Errorf("error parsing runningNodes info %q: %w", data, err)
	}

	info.LiveNodes = make([]models.HDFSNodeInfo, 0, len(nodesMap))
	info.DecommissioningNodes = make([]models.HDFSNodeInfo, 0, len(nodesMap))
	info.DecommissionedNodes = make([]models.HDFSNodeInfo, 0, len(nodesMap))
	info.DeadNodes = make([]models.HDFSNodeInfo, 0, len(nodesMap))

	for name, node := range nodesMap {
		node.Name = removePort(name)
		switch node.State {
		case "In Service":
			info.LiveNodes = append(info.LiveNodes, node)
		case "Decommission In Progress", "Entering Maintenance":
			info.DecommissioningNodes = append(info.DecommissioningNodes, node)
		case "Decommissioned", "In Maintenance":
			info.DecommissionedNodes = append(info.DecommissionedNodes, node)
		default:
			return models.HDFSInfo{}, xerrors.Errorf("unexpected node %s state %s", node.Name, node.State)
		}
	}
	data = []byte(nodes.DeadNodesRaw)
	nodesMap = make(map[string]models.HDFSNodeInfo)
	if err = json.Unmarshal(data, &nodesMap); err != nil {
		return models.HDFSInfo{}, xerrors.Errorf("error parsing nodes info %q: %w", data, err)
	}
	for name, node := range nodesMap {
		node.Name = removePort(name)
		info.DeadNodes = append(info.DeadNodes, node)
	}
	return info, nil
}

func parseHdfsResponse(body []byte) (models.HDFSInfo, error) {
	var err error
	var jmx hdfsResp
	var jmxNodes hdfsRespNodes

	if err = json.Unmarshal(body, &jmx); err != nil {
		return models.HDFSInfo{}, xerrors.Errorf("parse error %q: %w", body, err)
	}
	if len(jmx.Beans) == 0 {
		return models.HDFSInfo{}, xerrors.New("no bean found, possibly bad request")
	}
	if len(jmx.Beans) > 1 {
		return models.HDFSInfo{}, xerrors.Errorf("invalid amount of beans %d, specify your request", len(jmx.Beans))
	}
	info := jmx.Beans[0]

	if err = json.Unmarshal(body, &jmxNodes); err != nil {
		return models.HDFSInfo{}, xerrors.Errorf("nodes parse error %q: %w", body, err)
	}
	if len(jmx.Beans) == 0 {
		return models.HDFSInfo{}, xerrors.New("no bean found, possibly bad request")
	}
	if len(jmx.Beans) > 1 {
		return models.HDFSInfo{}, xerrors.Errorf("invalid amount of beans %d, specify your request", len(jmx.Beans))
	}
	nodesBean := jmxNodes.Beans[0]
	info, err = parseNodes(info, nodesBean)
	if err != nil {
		return models.HDFSInfo{}, err
	}
	info.Available = true
	return info, nil
}

// FetchHDFSInfo fetches and parses info from hadoop namenode
func FetchHDFSInfo(ctx context.Context, url string) (models.HDFSInfo, error) {
	client := &http.Client{}
	url = url + "/jmx?qry=Hadoop:service=NameNode,name=NameNodeInfo"
	request, err := http.NewRequest("GET", url, nil)
	if err != nil {
		return models.HDFSInfo{}, xerrors.Errorf("failed to build request: %w", err)
	}
	request = request.WithContext(ctx)

	resp, err := client.Do(request)
	if err != nil {
		return models.HDFSInfo{}, xerrors.Errorf("failed to get status from server %q: %w", url, err)
	}
	defer func() { _ = resp.Body.Close() }()
	if resp.StatusCode != http.StatusOK {
		return models.HDFSInfo{}, xerrors.Errorf("server responded with an error: %s", resp.Status)
	}
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return models.HDFSInfo{}, xerrors.Errorf("failed to read response body: %w", err)
	}

	return parseHdfsResponse(body)
}
