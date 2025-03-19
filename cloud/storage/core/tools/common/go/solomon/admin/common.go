package admin

import (
	"bytes"
	"errors"
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"text/template"

	solomon "a.yandex-team.ru/cloud/storage/core/tools/common/go/solomon/sdk"
)

////////////////////////////////////////////////////////////////////////////////

type Options struct {
	Cluster string

	ApplyChanges bool
	Remove       bool
	ShowDiff     bool

	AlertsPath     string
	ChannelsPath   string
	ClustersPath   string
	ServicesPath   string
	ShardsPath     string
	DashboardsPath string
	GraphsPath     string
}

var (
	colorRed    = "\033[31m"
	colorGreen  = "\033[32m"
	colorYellow = "\033[33m"
	colorReset  = "\033[0m"
)

////////////////////////////////////////////////////////////////////////////////

type Defaults struct {
	AlertState        string `yaml:"alert_state"`
	AlertWindowSecs   uint   `yaml:"alert_window_secs"`
	AlertDelaySecs    uint   `yaml:"alert_delay_secs"`
	ClusterHostDomain string `yaml:"cluster_host_domain"`
}

type ClusterConfig struct {
	ID               string `yaml:"id"`
	Name             string `yaml:"name"`
	SolomonCluster   string `yaml:"solomon_cluster"`
	Cluster          string `yaml:"cluster"`
	Zone             string `yaml:"zone"`
	Hosts            string `yaml:"hosts"`
	HostDomain       string `yaml:"host_domain"`
	SvmDomain        string `yaml:"svm_domain"`
	SolomonProjectID string `yaml:"solomon_project_id"`
	AuthType         string `yaml:"auth_by_iam"`
}

type ServiceConfig struct {
	SolomonProjectID string           `yaml:"solomon_project_id"`
	SolomonURL       string           `yaml:"solomon_url"`
	SolomonAuthType  solomon.AuthType `yaml:"solomon_auth_type"`
	Default          Defaults         `yaml:"defaults"`
	Clusters         []ClusterConfig  `yaml:"clusters"`
}

func (s *ServiceConfig) defaultClusterConfig() *ClusterConfig {
	return &ClusterConfig{
		SolomonProjectID: s.SolomonProjectID,
		HostDomain:       s.Default.ClusterHostDomain,
	}
}

func PrepareClusterConfigs(serviceConfig *ServiceConfig) error {
	defaults := &serviceConfig.Default

	for i := range serviceConfig.Clusters {
		cluster := &serviceConfig.Clusters[i]

		if len(cluster.SolomonProjectID) == 0 {
			cluster.SolomonProjectID = serviceConfig.SolomonProjectID
		}
		if len(cluster.HostDomain) == 0 {
			cluster.HostDomain = defaults.ClusterHostDomain
		}
	}
	return nil
}

func readTmplString(
	tmplString string,
	clusterConfig *ClusterConfig,
) (string, error) {

	tmpl, err := template.New("tmpl").Parse(tmplString)
	if err != nil {
		return "", err
	}

	type templateData struct {
		ID               string
		Name             string
		SolomonCluster   string
		Cluster          string
		Env              string
		Zone             string
		Hosts            string
		HostDomain       string
		SvmDomain        string
		SolomonProjectID string
	}

	var stringBuilder strings.Builder
	if clusterConfig != nil {
		err = tmpl.Execute(&stringBuilder, templateData{
			clusterConfig.ID,
			clusterConfig.Name,
			clusterConfig.SolomonCluster,
			clusterConfig.Cluster,
			strings.ToUpper(clusterConfig.Cluster),
			clusterConfig.Zone,
			clusterConfig.Hosts,
			clusterConfig.HostDomain,
			clusterConfig.SvmDomain,
			clusterConfig.SolomonProjectID,
		})
	} else {
		err = tmpl.Execute(&stringBuilder, nil)
	}

	if err != nil {
		return "", err
	}

	return stringBuilder.String(), nil
}

func readTmplStringArr(
	tmplStrings []string,
	clusterConfig *ClusterConfig,
) ([]string, error) {

	result := make([]string, 0)
	for _, tmplStr := range tmplStrings {
		res, err := readTmplString(tmplStr, clusterConfig)
		if err != nil {
			return nil, err
		}

		result = append(result, res)
	}

	return result, nil
}

func printDiff(id string, old []byte, new []byte) error {
	tmpFile, err := ioutil.TempFile(os.TempDir(), fmt.Sprintf("%v-", id))
	if err != nil {
		return err
	}
	defer os.Remove(tmpFile.Name())

	_, err = tmpFile.Write(old)
	if err != nil {
		return err
	}

	cmd := exec.Command("diff", "-du", tmpFile.Name(), "-")
	cmd.Stdin = bytes.NewReader(new)
	var out bytes.Buffer
	cmd.Stdout = &out

	err = cmd.Run()
	if err != nil {
		var ee *exec.ExitError
		if errors.As(err, &ee) && ee.ExitCode() == 1 {
			// Diff returns 1 if files differ.
			fmt.Println(out.String())
		} else {
			// Exec error.
			return err
		}
	}

	return nil
}

func listFiles(path string) ([]string, error) {
	fileInfo, err := os.Stat(path)
	if err != nil {
		return nil, err
	}

	if !fileInfo.IsDir() {
		return []string{path}, nil
	}

	files := make([]string, 0)

	err = filepath.Walk(path, func(filePath string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		if info.IsDir() {
			return nil
		}

		if strings.HasPrefix(filePath, ".") {
			return nil
		}

		files = append(files, filePath)

		return nil
	})
	if err != nil {
		return nil, err
	}

	return files, nil
}
