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

type DashboardParameters struct {
	Name  string `yaml:"name"`
	Value string `yaml:"value"`
}

type DashboardPanels struct {
	Colspan  uint   `yaml:"colspan"`
	Markdown string `yaml:"markdown"`
	Rowspan  uint   `yaml:"rowspan"`
	Subtitle string `yaml:"subtitle"`
	Title    string `yaml:"title"`
	Type     string `yaml:"type"`
	URL      string `yaml:"url"`
}

type DashboardRows struct {
	Panels []DashboardPanels `yaml:"panels"`
}

type DashboardConfig struct {
	ID       string   `yaml:"id"`
	Clusters []string `yaml:"clusters"`
	Name     string   `yaml:"name"`

	Description      string                `yaml:"description"`
	HeightMultiplier float32               `yaml:"height_multiplier"`
	Parameters       []DashboardParameters `yaml:"parameters"`
	Rows             []DashboardRows       `yaml:"rows"`
}

type DashboardsConfigs struct {
	Dashboards []DashboardConfig `yaml:"dashboards"`
}

////////////////////////////////////////////////////////////////////////////////

func loadDashboardsConfigsFromFile(filePath string) ([]DashboardConfig, error) {
	configData, err := ioutil.ReadFile(filePath)
	if err != nil {
		return nil, err
	}

	dashboardsConfigs := &DashboardsConfigs{}
	if err := yaml.Unmarshal([]byte(string(configData)), dashboardsConfigs); err != nil {
		return nil, err
	}

	return dashboardsConfigs.Dashboards, nil
}

func loadDashboardsConfigsFromPath(path string) ([]DashboardConfig, error) {
	files, err := listFiles(path)
	if err != nil {
		return nil, err
	}

	var configs []DashboardConfig

	for _, file := range files {
		dashboardConfigs, err := loadDashboardsConfigsFromFile(file)
		if err != nil {
			return nil, err
		}

		configs = append(configs, dashboardConfigs...)
	}

	return configs, nil
}

////////////////////////////////////////////////////////////////////////////////

func createSolomonDashboard(
	dashboardConfig DashboardConfig,
	clusterConfig *ClusterConfig,
	solomonProjectID string,
) (solomon.Dashboard, error) {

	var err error

	var dashboard solomon.Dashboard
	dashboard.ProjectID = solomonProjectID

	dashboard.ID, err = readTmplString(dashboardConfig.ID, clusterConfig)
	if err != nil {
		return solomon.Dashboard{}, err
	}

	dashboard.Name, err = readTmplString(dashboardConfig.Name, clusterConfig)
	if err != nil {
		return solomon.Dashboard{}, err
	}

	dashboard.HeightMultiplier = dashboardConfig.HeightMultiplier

	dashboard.Parameters = make([]solomon.DashboardParameters, 0)
	for _, param := range dashboardConfig.Parameters {
		p := solomon.DashboardParameters{}

		p.Name, err = readTmplString(param.Name, clusterConfig)
		if err != nil {
			return solomon.Dashboard{}, err
		}

		p.Value, err = readTmplString(param.Value, clusterConfig)
		if err != nil {
			return solomon.Dashboard{}, err
		}

		dashboard.Parameters = append(dashboard.Parameters, p)
	}

	dashboard.Rows = make([]solomon.DashboardRows, 0)
	for _, row := range dashboardConfig.Rows {
		r := solomon.DashboardRows{}

		for _, panel := range row.Panels {
			p := solomon.DashboardPanels{}

			p.Type, err = readTmplString(panel.Type, clusterConfig)
			if err != nil {
				return solomon.Dashboard{}, err
			}

			p.Title, err = readTmplString(panel.Title, clusterConfig)
			if err != nil {
				return solomon.Dashboard{}, err
			}

			p.Subtitle, err = readTmplString(panel.Subtitle, clusterConfig)
			if err != nil {
				return solomon.Dashboard{}, err
			}

			p.URL, err = readTmplString(panel.URL, clusterConfig)
			if err != nil {
				return solomon.Dashboard{}, err
			}

			p.Markdown, err = readTmplString(panel.Markdown, clusterConfig)
			if err != nil {
				return solomon.Dashboard{}, err
			}

			p.Colspan = panel.Colspan
			p.Rowspan = panel.Rowspan

			r.Panels = append(r.Panels, p)
		}

		dashboard.Rows = append(dashboard.Rows, r)
	}

	return dashboard, nil
}

func createSolomonDashboards(
	dashboardConfigs []DashboardConfig,
	serviceConfig *ServiceConfig,
	opts *Options,
) ([]solomon.Dashboard, error) {

	projectID := serviceConfig.SolomonProjectID

	dashboardIds := make(map[string]bool)
	var dashboards []solomon.Dashboard

	for _, dashboardConfig := range dashboardConfigs {
		if dashboardConfig.Clusters == nil {
			if len(opts.Cluster) != 0 {
				continue
			}

			dashboard, err := createSolomonDashboard(dashboardConfig, serviceConfig.defaultClusterConfig(), projectID)
			if err != nil {
				return nil, err
			}

			if _, ok := dashboardIds[dashboard.ID]; ok {
				return nil, fmt.Errorf("multiple dashboards with the same ID: %v", dashboard.ID)
			}
			dashboardIds[dashboard.ID] = false
			dashboards = append(dashboards, dashboard)
			continue
		}

		for _, clusterID := range dashboardConfig.Clusters {
			if len(opts.Cluster) != 0 && opts.Cluster != clusterID {
				continue
			}

			for _, cluster := range serviceConfig.Clusters {
				if clusterID == cluster.ID {
					dashboard, err := createSolomonDashboard(dashboardConfig, &cluster, projectID)
					if err != nil {
						return nil, err
					}

					if _, ok := dashboardIds[dashboard.ID]; ok {
						return nil, fmt.Errorf("multiple dashboards with the same ID: %v", dashboard.ID)
					}
					dashboardIds[dashboard.ID] = false
					dashboards = append(dashboards, dashboard)
					break
				}
			}
		}
	}
	return dashboards, nil
}

func loadSolomonDashboardIds(
	ctx context.Context,
	solomonClient solomon.SolomonClientIface,
) (map[string]bool, error) {

	solomonDashboardIds, err := solomonClient.ListDashboards(ctx)
	if err != nil {
		return nil, err
	}

	solomonDashboards := make(map[string]bool)

	for _, dashboardID := range solomonDashboardIds {
		solomonDashboards[dashboardID] = false
	}

	return solomonDashboards, nil
}

////////////////////////////////////////////////////////////////////////////////

type DashboardManager struct {
	opts   *Options
	config *ServiceConfig

	solomonClient solomon.SolomonClientIface
}

func (a *DashboardManager) Run(ctx context.Context) error {
	dashboardConfigs, err := loadDashboardsConfigsFromPath(a.opts.DashboardsPath)
	if err != nil {
		return err
	}

	dashboards, err := createSolomonDashboards(dashboardConfigs, a.config, a.opts)
	if err != nil {
		return err
	}

	solomonDashboardIds, err := loadSolomonDashboardIds(ctx, a.solomonClient)
	if err != nil {
		return err
	}

	addedDashboardCount := 0
	changedDashboardCount := 0
	unchangedDashboardCount := 0
	for _, dashboard := range dashboards {
		if _, ok := solomonDashboardIds[dashboard.ID]; ok {
			solomonDashboardIds[dashboard.ID] = true

			solomonDashboard, err := a.solomonClient.GetDashboard(ctx, dashboard.ID)
			if err != nil {
				return err
			}

			dashboard.Version = solomonDashboard.Version

			if reflect.DeepEqual(solomonDashboard, dashboard) {
				unchangedDashboardCount++
			} else {
				changedDashboardCount++
				if a.opts.ApplyChanges {
					fmt.Println("Update dashboard: ", dashboard.ID)
					_, err := a.solomonClient.UpdateDashboard(ctx, dashboard)
					if err != nil {
						return err
					}
				} else {
					fmt.Println("Need to update dashboard: ", dashboard.ID)
				}

				if a.opts.ShowDiff {
					oldDashboard, err := json.MarshalIndent(solomonDashboard, "", "  ")
					if err != nil {
						return err
					}

					newDashboard, err := json.MarshalIndent(dashboard, "", "  ")
					if err != nil {
						return err
					}

					err = printDiff(dashboard.ID, oldDashboard, newDashboard)
					if err != nil {
						return err
					}
				}
			}

		} else {
			addedDashboardCount++
			if a.opts.ApplyChanges {
				fmt.Println("Add dashboard: ", dashboard.ID)
				_, err := a.solomonClient.AddDashboard(ctx, dashboard)
				if err != nil {
					return err
				}
			} else {
				fmt.Println("Need to add dashboard: ", dashboard.ID)
			}
		}
	}

	var untrackedDashboardIds []string
	for dashboardID, tracked := range solomonDashboardIds {
		if tracked {
			continue
		}

		untrackedDashboardIds = append(untrackedDashboardIds, dashboardID)
	}

	if a.opts.Remove {
		sort.Strings(untrackedDashboardIds)
		for _, dashboardID := range untrackedDashboardIds {
			if a.opts.ApplyChanges {
				fmt.Println("Delete dashboard: ", dashboardID)
				err = a.solomonClient.DeleteDashboard(ctx, dashboardID)
				if err != nil {
					return err
				}
			} else {
				fmt.Println("Need to delete dashboard: ", dashboardID)
			}
		}
	}

	if addedDashboardCount != 0 {
		fmt.Println(colorGreen+"Added dashboards count:", addedDashboardCount, colorReset)
	}

	if changedDashboardCount != 0 {
		fmt.Println(colorYellow+"Changed dashboards count:", changedDashboardCount, colorReset)
	}

	fmt.Println("Unchanged dashboards count:", unchangedDashboardCount)

	if !a.opts.Remove && len(untrackedDashboardIds) != 0 {
		fmt.Printf(colorRed+"Untracked dashboards count:%d, %v\n"+colorReset, len(untrackedDashboardIds), untrackedDashboardIds)
	}
	return nil
}

////////////////////////////////////////////////////////////////////////////////

func NewDashboardManager(
	opts *Options,
	config *ServiceConfig,
	solomonClient solomon.SolomonClientIface,
) *DashboardManager {

	return &DashboardManager{
		opts:          opts,
		config:        config,
		solomonClient: solomonClient,
	}
}
