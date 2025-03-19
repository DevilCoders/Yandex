package admin

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"reflect"
	"sort"

	"gopkg.in/yaml.v2"

	solomon "a.yandex-team.ru/cloud/storage/core/tools/common/go/solomon/sdk"
)

////////////////////////////////////////////////////////////////////////////////

type ClustersConductorGroup struct {
	Group  string   `yaml:"group"`
	Labels []string `yaml:"labels"`
}

type ClustersCloudDNS struct {
	Env    string   `yaml:"env"`
	Name   string   `yaml:"name"`
	Labels []string `yaml:"labels"`
}

type Host struct {
	URLPattern string   `yaml:"urlPattern"`
	Ranges     string   `yaml:"ranges"`
	DC         string   `yaml:"dc"`
	Labels     []string `yaml:"labels"`
}

type ClustersConfig struct {
	ID       string   `yaml:"id"`
	Clusters []string `yaml:"clusters"`
	Name     string   `yaml:"name"`

	Hosts           []Host                   `yaml:"hosts"`
	ConductorGroups []ClustersConductorGroup `yaml:"conductor_groups"`
	CloudDNS        []ClustersCloudDNS       `yaml:"cloud_dns"`
	SensorsTTLDays  uint                     `yaml:"sensors_ttl_days"`
	UseFqdn         bool                     `yaml:"use_fqdn"`
}

type ClusterConfigs struct {
	Clusters []ClustersConfig `yaml:"clusters"`
}

////////////////////////////////////////////////////////////////////////////////

func loadClusterConfigsFromFile(filePath string) ([]ClustersConfig, error) {
	configData, err := ioutil.ReadFile(filePath)
	if err != nil {
		return nil, err
	}

	clusterConfigs := &ClusterConfigs{}
	if err := yaml.Unmarshal([]byte(string(configData)), clusterConfigs); err != nil {
		return nil, err
	}

	return clusterConfigs.Clusters, nil
}

func loadClusterConfigsFromPath(path string) ([]ClustersConfig, error) {
	files, err := listFiles(path)
	if err != nil {
		return nil, err
	}

	var configs []ClustersConfig

	for _, file := range files {
		clusterConfigs, err := loadClusterConfigsFromFile(file)
		if err != nil {
			return nil, err
		}

		configs = append(configs, clusterConfigs...)
	}

	return configs, nil
}

////////////////////////////////////////////////////////////////////////////////

func createSolomonCluster(
	clustersConfig ClustersConfig,
	clusterConfig *ClusterConfig,
	solomonProjectID string,
) (solomon.Cluster, error) {

	var err error

	var cluster solomon.Cluster
	cluster.ProjectID = solomonProjectID

	cluster.ID, err = readTmplString(clustersConfig.ID, clusterConfig)
	if err != nil {
		return solomon.Cluster{}, err
	}

	cluster.Name, err = readTmplString(clustersConfig.Name, clusterConfig)
	if err != nil {
		return solomon.Cluster{}, err
	}

	for _, conductorGroup := range clustersConfig.ConductorGroups {

		group := solomon.ClusterConductorGroup{}

		group.Group, err = readTmplString(conductorGroup.Group, clusterConfig)
		if err != nil {
			return solomon.Cluster{}, err
		}

		group.Labels, err = readTmplStringArr(conductorGroup.Labels, clusterConfig)
		if err != nil {
			return solomon.Cluster{}, err
		}

		cluster.ConductorGroups = append(cluster.ConductorGroups, group)
	}

	for _, cloudDNS := range clustersConfig.CloudDNS {

		dns := solomon.ClustersCloudDNS{}

		dns.Env, err = readTmplString(cloudDNS.Env, clusterConfig)
		if err != nil {
			return solomon.Cluster{}, err
		}

		dns.Name, err = readTmplString(cloudDNS.Name, clusterConfig)
		if err != nil {
			return solomon.Cluster{}, err
		}

		dns.Labels, err = readTmplStringArr(cloudDNS.Labels, clusterConfig)
		if err != nil {
			return solomon.Cluster{}, err
		}

		cluster.CloudDNS = append(cluster.CloudDNS, dns)
	}

	for _, clusterHost := range clustersConfig.Hosts {
		host := solomon.Host{}

		host.URLPattern, err = readTmplString(clusterHost.URLPattern, clusterConfig)
		if err != nil {
			return solomon.Cluster{}, err
		}

		host.Ranges, err = readTmplString(clusterHost.Ranges, clusterConfig)
		if err != nil {
			return solomon.Cluster{}, err
		}

		host.DC, err = readTmplString(clusterHost.DC, clusterConfig)
		if err != nil {
			return solomon.Cluster{}, err
		}

		host.Labels = make([]string, 0)
		for _, clusterLabel := range clusterHost.Labels {
			label, err := readTmplString(clusterLabel, clusterConfig)
			if err != nil {
				return solomon.Cluster{}, err
			}

			clusterHost.Labels = append(clusterHost.Labels, label)
		}

		cluster.Hosts = append(cluster.Hosts, host)
	}

	cluster.SensorsTTLDays = clustersConfig.SensorsTTLDays
	cluster.UseFqdn = clustersConfig.UseFqdn

	return cluster, nil
}

func createSolomonClusters(
	clustersConfigs []ClustersConfig,
	serviceConfig *ServiceConfig,
	opts *Options,
) ([]solomon.Cluster, error) {

	projectID := serviceConfig.SolomonProjectID

	clusterIds := make(map[string]bool)
	var clusters []solomon.Cluster

	for _, clusterConfig := range clustersConfigs {
		if clusterConfig.Clusters == nil {
			if len(opts.Cluster) != 0 {
				continue
			}

			cluster, err := createSolomonCluster(clusterConfig, serviceConfig.defaultClusterConfig(), projectID)
			if err != nil {
				return nil, err
			}

			if _, ok := clusterIds[cluster.ID]; ok {
				return nil, fmt.Errorf("multiple clusters with the same ID: %v", cluster.ID)
			}
			clusterIds[cluster.ID] = false
			clusters = append(clusters, cluster)
			continue
		}

		for _, clusterID := range clusterConfig.Clusters {
			if len(opts.Cluster) != 0 && opts.Cluster != clusterID {
				continue
			}

			for _, cluster := range serviceConfig.Clusters {
				if clusterID == cluster.ID {
					solomonCluster, err := createSolomonCluster(clusterConfig, &cluster, projectID)
					if err != nil {
						return nil, err
					}

					if _, ok := clusterIds[solomonCluster.ID]; ok {
						return nil, fmt.Errorf("multiple clusters with the same ID: %v", solomonCluster.ID)
					}
					clusterIds[solomonCluster.ID] = false
					clusters = append(clusters, solomonCluster)
					break
				}
			}
		}
	}
	return clusters, nil
}

func loadSolomonClusterIds(
	ctx context.Context,
	solomonClient solomon.SolomonClientIface,
) (map[string]bool, error) {

	solomonClusterIds, err := solomonClient.ListClusters(ctx)
	if err != nil {
		return nil, err
	}

	solomonClusters := make(map[string]bool)

	for _, clusterID := range solomonClusterIds {
		solomonClusters[clusterID] = false
	}

	return solomonClusters, nil
}

////////////////////////////////////////////////////////////////////////////////

type ClusterManager struct {
	opts   *Options
	config *ServiceConfig

	solomonClient solomon.SolomonClientIface
}

func (a *ClusterManager) Run(ctx context.Context) error {
	clustersConfigs, err := loadClusterConfigsFromPath(a.opts.ClustersPath)
	if err != nil {
		return err
	}

	clusters, err := createSolomonClusters(clustersConfigs, a.config, a.opts)
	if err != nil {
		return err
	}

	solomonClusterIds, err := loadSolomonClusterIds(ctx, a.solomonClient)
	if err != nil {
		return err
	}

	addedClusterCount := 0
	changedClusterCount := 0
	unchangedClusterCount := 0
	for _, cluster := range clusters {
		if _, ok := solomonClusterIds[cluster.ID]; ok {
			solomonClusterIds[cluster.ID] = true

			solomonCluster, err := a.solomonClient.GetCluster(ctx, cluster.ID)
			if err != nil {
				return err
			}

			cluster.Version = solomonCluster.Version

			if reflect.DeepEqual(solomonCluster, cluster) {
				unchangedClusterCount++
			} else {
				changedClusterCount++
				if a.opts.ApplyChanges {
					fmt.Println("Update cluster: ", cluster.ID)
					_, err := a.solomonClient.UpdateCluster(ctx, cluster)
					if err != nil {
						return err
					}
				} else {
					fmt.Println("Need to update cluster: ", cluster.ID)
				}

				if a.opts.ShowDiff {
					oldCluster, err := json.MarshalIndent(solomonCluster, "", "  ")
					if err != nil {
						return err
					}

					newCluster, err := json.MarshalIndent(cluster, "", "  ")
					if err != nil {
						return err
					}

					err = printDiff(cluster.ID, oldCluster, newCluster)
					if err != nil {
						return err
					}
				}
			}

		} else {
			addedClusterCount++
			if a.opts.ApplyChanges {
				fmt.Println("Add cluster: ", cluster.ID)
				_, err := a.solomonClient.AddCluster(ctx, cluster)
				if err != nil {
					return err
				}
			} else {
				fmt.Println("Need to add cluster: ", cluster.ID)
			}
		}
	}

	var untrackedClusterIds []string
	for clusterID, tracked := range solomonClusterIds {
		if tracked {
			continue
		}

		untrackedClusterIds = append(untrackedClusterIds, clusterID)
	}

	if a.opts.Remove {
		sort.Strings(untrackedClusterIds)
		for _, clusterID := range untrackedClusterIds {
			if a.opts.ApplyChanges {
				fmt.Println("Delete cluster: ", clusterID)
				err = a.solomonClient.DeleteCluster(ctx, clusterID)
				if err != nil {
					return err
				}
			} else {
				fmt.Println("Need to delete cluster: ", clusterID)
			}
		}
	}

	if addedClusterCount != 0 {
		fmt.Println(colorGreen+"Added clusters count:", addedClusterCount, colorReset)
	}

	if changedClusterCount != 0 {
		fmt.Println(colorYellow+"Changed clusters count:", changedClusterCount, colorReset)
	}

	fmt.Println("Unchanged clusters count:", unchangedClusterCount)

	if !a.opts.Remove && len(untrackedClusterIds) != 0 {
		fmt.Printf(colorRed+"Untracked clusters count:%d %v\n"+colorReset, len(untrackedClusterIds), untrackedClusterIds)
	}
	return nil
}

////////////////////////////////////////////////////////////////////////////////

func NewClusterManager(
	opts *Options,
	config *ServiceConfig,
	solomonClient solomon.SolomonClientIface,
) *ClusterManager {

	return &ClusterManager{
		opts:          opts,
		config:        config,
		solomonClient: solomonClient,
	}
}
