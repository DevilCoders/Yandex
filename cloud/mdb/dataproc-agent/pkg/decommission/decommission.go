package decommission

import (
	"bufio"
	"bytes"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"

	"a.yandex-team.ru/cloud/mdb/dataproc-agent/pkg/models"
	"a.yandex-team.ru/library/go/core/log"
)

type DecommissionConfig struct {
	YARNExcludeNodesConfigPath string `json:"yarn_exclude_nodes_config_path" yaml:"yarn_exclude_nodes_config_path"`
	HDFSExcludeNodesConfigPath string `json:"hdfs_exclude_nodes_config_path" yaml:"hdfs_exclude_nodes_config_path"`
}

func runCommandAsUser(user string, args ...string) (string, string, error) {
	var stderr bytes.Buffer
	commandParts := append([]string{"-u", user}, args...)
	cmd := exec.Command(
		"sudo", commandParts...,
	)
	cmd.Stderr = &stderr
	output, err := cmd.Output()
	return string(output), stderr.String(), err
}

func getYarnNodesAvailableForDecommission(info models.Info) map[string]struct{} {
	yarnNodes := make(map[string]struct{})
	for _, node := range info.YARN.LiveNodes {
		if node.State == "RUNNING" || node.State == "DECOMMISSIONING" {
			yarnNodes[node.Name] = struct{}{}
		}
	}
	return yarnNodes
}

func getHdfsNodesAvailableForDecommission(info models.Info) map[string]struct{} {
	hdfsNodes := make(map[string]struct{})
	for _, node := range info.HDFS.LiveNodes {
		if node.State == "In Service" || node.State == "Decommission In Progress" {
			hdfsNodes[node.Name] = struct{}{}
		}
	}
	return hdfsNodes
}

type Decommissioner struct {
	Logger                         log.Logger
	Config                         DecommissionConfig
	YarnRequestedDecommissionHosts []string
	HdfsRequestedDecommissionHosts []string
}

func (d *Decommissioner) logIfError(err error) {
	if err != nil {
		d.Logger.Error(err.Error())
	}
}

func (d *Decommissioner) writeFile(filePath string, hosts map[string]struct{}) error {
	f, err := ioutil.TempFile("", filepath.Base(filePath))
	if err != nil {
		return err
	}
	w := bufio.NewWriter(f)
	for host := range hosts {
		_, err := fmt.Fprintln(w, host)
		if err != nil {
			return err
		}
	}
	err = w.Flush()
	if err != nil {
		return err
	}
	_ = f.Close()
	err = os.Chmod(f.Name(), 0644)
	if err != nil {
		return err
	}
	err = os.Rename(f.Name(), filePath)
	if err != nil {
		return err
	}
	return nil
}

func getMapKeys(dictionary map[string]struct{}) []string {
	keys := make([]string, 0, len(dictionary))
	for key := range dictionary {
		keys = append(keys, key)
	}
	return keys
}

func (d *Decommissioner) decommissionYarnNodes(yarnNodes map[string]struct{}, hosts []string, timeout int) error {
	yarnExcludeNodesConfigPath := d.Config.YARNExcludeNodesConfigPath
	hostsToExclude := make(map[string]struct{})

	// remove previously decommissioned nodes from Decommission list
	for _, host := range hosts {
		if _, exists := yarnNodes[host]; exists {
			hostsToExclude[host] = struct{}{}
		}
	}

	d.Logger.Infof("Decommissioning YARN nodes %s with timeout %d", hostsToExclude, timeout)
	if len(hostsToExclude) > 0 {
		err := d.writeFile(yarnExcludeNodesConfigPath, hostsToExclude)
		if err != nil {
			return err
		}
	}

	_, stderr, err := runCommandAsUser("yarn",
		"yarn", "rmadmin", "-refreshNodes",
		"-graceful", fmt.Sprint(timeout),
		"-server",
	)
	if err != nil {
		d.Logger.Error(stderr)
	} else {
		d.YarnRequestedDecommissionHosts = getMapKeys(hostsToExclude)
	}
	err = os.Truncate(yarnExcludeNodesConfigPath, 0)
	return err
}

func (d *Decommissioner) decommissionHdfsNodes(hdfsNodes map[string]struct{}, hosts []string) error {
	hdfsExcludeNodesConfigPath := d.Config.HDFSExcludeNodesConfigPath
	hostsToExclude := make(map[string]struct{})

	// remove previously decommissioned nodes from Decommission list
	for _, host := range hosts {
		if _, exists := hdfsNodes[host]; exists {
			hostsToExclude[host] = struct{}{}
		}
	}

	if len(hostsToExclude) > 0 {
		d.Logger.Infof("Decommissioning HDFS nodes %s", hostsToExclude)
		err := d.writeFile(hdfsExcludeNodesConfigPath, hostsToExclude)
		if err != nil {
			return err
		}
		_, stderr, err := runCommandAsUser("hdfs",
			"hdfs", "dfsadmin", "-refreshNodes",
		)
		if err != nil {
			d.Logger.Error(stderr)
		} else {
			d.HdfsRequestedDecommissionHosts = getMapKeys(hostsToExclude)
		}
		err = os.Truncate(hdfsExcludeNodesConfigPath, 0)
		return err
	}
	return nil
}

func (d *Decommissioner) Decommission(info models.Info, nodesToDecommission *models.NodesToDecommission) {
	if nodesToDecommission == nil {
		return
	}
	// This condition intentionally differs for HDFS and YARN decommission
	// YARN and HDFS decommission have different results:
	//    After YARN decommission hadoop-yarn-nodemanager service becomes disabled on node
	//    and extra nodes refresh will not affect it until the node restarts.
	//    If we will not refresh YARN decommission nodes until the node restarts,
	//    node will start with disabled hadoop-yarn-nodemanager service what will make it unavailable for YARN.
	//    In case when YARN nodes are decommissioned without masternode restart (nodes update with downtime)
	//    the worker will send decommission request with empty yarn nodes list (after the last node is decommissioned)
	//    and non-empty timestamp.
	//
	//    After HDFS decommission hadoop-hdfs-datanode service will not be disabled,
	//    so extra nodes refresh without the node in decommission list
	//    will immediately commission node back, and this is an unwanted behavior.
	if nodesToDecommission.DecommissionTimeout != 0 {
		yarnNodes := getYarnNodesAvailableForDecommission(info)
		err := d.decommissionYarnNodes(
			yarnNodes,
			nodesToDecommission.YarnHostsToDecommission,
			nodesToDecommission.DecommissionTimeout,
		)
		d.logIfError(err)
	}
	if nodesToDecommission.HdfsHostsToDecommission != nil {
		hdfsNodes := getHdfsNodesAvailableForDecommission(info)
		err := d.decommissionHdfsNodes(
			hdfsNodes,
			nodesToDecommission.HdfsHostsToDecommission,
		)
		d.logIfError(err)
	}
}
