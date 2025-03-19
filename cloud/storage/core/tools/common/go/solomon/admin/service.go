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

type ServiceAggrRules struct {
	Cond     []string `yaml:"cond"`
	Function string   `yaml:"function,omitempty"`
	Target   []string `yaml:"target"`
}

type ServicePriorityRules struct {
	Priority int    `yaml:"priority"`
	Target   string `yaml:"target"`
}

type ServiceSensorConf struct {
	AggrRules      []ServiceAggrRules `yaml:"aggr_rules,omitempty"`
	RawDataMemOnly bool               `yaml:"raw_data_mem_only"`
}

type ServicesConfig struct {
	ID       string   `yaml:"id"`
	Clusters []string `yaml:"clusters"`
	Name     string   `yaml:"name"`

	AddTSArgs       bool              `yaml:"add_ts_args"`
	GridSec         int               `yaml:"grid_sec"`
	Interval        int               `yaml:"interval"`
	Path            string            `yaml:"path"`
	Port            int               `yaml:"port"`
	Protocol        string            `yaml:"protocol"`
	SensorConf      ServiceSensorConf `yaml:"sensor_conf,omitempty"`
	SensorNameLabel string            `yaml:"sensor_name_label"`
	SensorsTTLDays  int               `yaml:"sensors_ttl_days"`
	TvmDestID       string            `yaml:"tvm_dest_id"`
}

type ServicesConfigs struct {
	Services []ServicesConfig `yaml:"services"`
}

////////////////////////////////////////////////////////////////////////////////

func loadServicesConfigsFromFile(filePath string) ([]ServicesConfig, error) {
	configData, err := ioutil.ReadFile(filePath)
	if err != nil {
		return nil, err
	}

	servicesConfigs := &ServicesConfigs{}
	if err := yaml.Unmarshal([]byte(string(configData)), servicesConfigs); err != nil {
		return nil, err
	}

	return servicesConfigs.Services, nil
}

func loadServicesConfigsFromPath(path string) ([]ServicesConfig, error) {
	files, err := listFiles(path)
	if err != nil {
		return nil, err
	}

	var configs []ServicesConfig

	for _, file := range files {
		serviceConfigs, err := loadServicesConfigsFromFile(file)
		if err != nil {
			return nil, err
		}

		configs = append(configs, serviceConfigs...)
	}

	return configs, nil
}

////////////////////////////////////////////////////////////////////////////////

func createSolomonService(
	servicesConfig ServicesConfig,
	clusterConfig *ClusterConfig,
	solomonProjectID string,
) (solomon.Service, error) {

	var err error

	var services solomon.Service
	services.ProjectID = solomonProjectID

	services.ID, err = readTmplString(servicesConfig.ID, clusterConfig)
	if err != nil {
		return solomon.Service{}, err
	}

	services.Name, err = readTmplString(servicesConfig.Name, clusterConfig)
	if err != nil {
		return solomon.Service{}, err
	}

	services.Path, err = readTmplString(servicesConfig.Path, clusterConfig)
	if err != nil {
		return solomon.Service{}, err
	}

	services.Protocol, err = readTmplString(servicesConfig.Protocol, clusterConfig)
	if err != nil {
		return solomon.Service{}, err
	}

	services.SensorNameLabel, err = readTmplString(servicesConfig.SensorNameLabel, clusterConfig)
	if err != nil {
		return solomon.Service{}, err
	}

	services.TvmDestID, err = readTmplString(servicesConfig.TvmDestID, clusterConfig)
	if err != nil {
		return solomon.Service{}, err
	}

	services.SensorConf.RawDataMemOnly = servicesConfig.SensorConf.RawDataMemOnly

	if servicesConfig.SensorConf.AggrRules != nil {
		services.SensorConf.AggrRules = make([]solomon.ServiceAggrRules, 0)
	}
	for _, rule := range servicesConfig.SensorConf.AggrRules {
		r := solomon.ServiceAggrRules{}

		r.Function, err = readTmplString(rule.Function, clusterConfig)
		if err != nil {
			return solomon.Service{}, err
		}

		r.Cond, err = readTmplStringArr(rule.Cond, clusterConfig)
		if err != nil {
			return solomon.Service{}, err
		}

		r.Target, err = readTmplStringArr(rule.Target, clusterConfig)
		if err != nil {
			return solomon.Service{}, err
		}

		services.SensorConf.AggrRules = append(services.SensorConf.AggrRules, r)
	}

	services.AddTSArgs = servicesConfig.AddTSArgs
	services.GridSec = servicesConfig.GridSec
	services.Interval = servicesConfig.Interval
	services.Port = servicesConfig.Port
	services.SensorsTTLDays = servicesConfig.SensorsTTLDays

	return services, nil
}

func createSolomonServices(
	servicesConfigs []ServicesConfig,
	serviceConfig *ServiceConfig,
	opts *Options,
) ([]solomon.Service, error) {

	projectID := serviceConfig.SolomonProjectID

	servicesIds := make(map[string]bool)
	var services []solomon.Service

	for _, servicesConfig := range servicesConfigs {
		if servicesConfig.Clusters == nil {
			if len(opts.Cluster) != 0 {
				continue
			}

			service, err := createSolomonService(servicesConfig, serviceConfig.defaultClusterConfig(), projectID)
			if err != nil {
				return nil, err
			}

			if _, ok := servicesIds[service.ID]; ok {
				return nil, fmt.Errorf("multiple services with the same ID: %v", service.ID)
			}
			servicesIds[service.ID] = false
			services = append(services, service)
			continue
		}

		for _, clusterID := range servicesConfig.Clusters {
			if len(opts.Cluster) != 0 && opts.Cluster != clusterID {
				continue
			}

			for _, cluster := range serviceConfig.Clusters {
				if clusterID == cluster.ID {
					service, err := createSolomonService(servicesConfig, &cluster, projectID)
					if err != nil {
						return nil, err
					}

					if _, ok := servicesIds[service.ID]; ok {
						return nil, fmt.Errorf("multiple services with the same ID: %v", service.ID)
					}
					servicesIds[service.ID] = false
					services = append(services, service)
					break
				}
			}
		}
	}
	return services, nil
}

func loadSolomonServicesIds(
	ctx context.Context,
	solomonClient solomon.SolomonClientIface,
) (map[string]bool, error) {

	solomonServicesIds, err := solomonClient.ListServices(ctx)
	if err != nil {
		return nil, err
	}

	solomonServices := make(map[string]bool)

	for _, servicesID := range solomonServicesIds {
		solomonServices[servicesID] = false
	}

	return solomonServices, nil
}

////////////////////////////////////////////////////////////////////////////////

type ServicesManager struct {
	opts   *Options
	config *ServiceConfig

	solomonClient solomon.SolomonClientIface
}

func (a *ServicesManager) Run(ctx context.Context) error {
	servicesConfigs, err := loadServicesConfigsFromPath(a.opts.ServicesPath)
	if err != nil {
		return err
	}

	services, err := createSolomonServices(servicesConfigs, a.config, a.opts)
	if err != nil {
		return err
	}

	solomonServicesIds, err := loadSolomonServicesIds(ctx, a.solomonClient)
	if err != nil {
		return err
	}

	addedServicesCount := 0
	changedServicesCount := 0
	unchangedServicesCount := 0
	for _, service := range services {
		if _, ok := solomonServicesIds[service.ID]; ok {
			solomonServicesIds[service.ID] = true

			solomonServices, err := a.solomonClient.GetService(ctx, service.ID)
			if err != nil {
				return err
			}

			service.Version = solomonServices.Version

			if reflect.DeepEqual(solomonServices, service) {
				unchangedServicesCount++
			} else {
				changedServicesCount++
				if a.opts.ApplyChanges {
					fmt.Println("Update service: ", service.ID)
					_, err := a.solomonClient.UpdateService(ctx, service)
					if err != nil {
						return err
					}
				} else {
					fmt.Println("Need to update service: ", service.ID)
				}

				if a.opts.ShowDiff {
					oldServices, err := json.MarshalIndent(solomonServices, "", "  ")
					if err != nil {
						return err
					}

					newServices, err := json.MarshalIndent(service, "", "  ")
					if err != nil {
						return err
					}

					err = printDiff(service.ID, oldServices, newServices)
					if err != nil {
						return err
					}
				}
			}

		} else {
			addedServicesCount++
			if a.opts.ApplyChanges {
				fmt.Println("Add service: ", service.ID)
				_, err := a.solomonClient.AddService(ctx, service)
				if err != nil {
					return err
				}
			} else {
				fmt.Println("Need to add service: ", service.ID)
			}
		}
	}

	var untrackedServicesIds []string
	for servicesID, tracked := range solomonServicesIds {
		if tracked {
			continue
		}

		untrackedServicesIds = append(untrackedServicesIds, servicesID)
	}

	if a.opts.Remove {
		sort.Strings(untrackedServicesIds)
		for _, servicesID := range untrackedServicesIds {
			if a.opts.ApplyChanges {
				fmt.Println("Delete service: ", servicesID)
				err = a.solomonClient.DeleteService(ctx, servicesID)
				if err != nil {
					return err
				}
			} else {
				fmt.Println("Need to delete service: ", servicesID)
			}
		}
	}

	if addedServicesCount != 0 {
		fmt.Println(colorGreen+"Added services count:", addedServicesCount, colorReset)
	}

	if changedServicesCount != 0 {
		fmt.Println(colorYellow+"Changed services count:", changedServicesCount, colorReset)
	}

	fmt.Println("Unchanged services count:", unchangedServicesCount)

	if !a.opts.Remove && len(untrackedServicesIds) != 0 {
		fmt.Printf(colorRed+"Untracked services count:%d, %v\n"+colorReset, len(untrackedServicesIds), untrackedServicesIds)
	}
	return nil
}

////////////////////////////////////////////////////////////////////////////////

func NewServicesManager(
	opts *Options,
	config *ServiceConfig,
	solomonClient solomon.SolomonClientIface,
) *ServicesManager {

	return &ServicesManager{
		opts:          opts,
		config:        config,
		solomonClient: solomonClient,
	}
}
