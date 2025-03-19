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

type GraphID = string

type GraphParameters struct {
	Name  string `json:"name" yaml:"name"`
	Value string `json:"value" yaml:"value"`
}

type GraphSelectors struct {
	Name  string `json:"name" yaml:"name"`
	Value string `json:"value" yaml:"value"`
}

type GraphElements struct {
	Title     string `json:"title" yaml:"title"`
	Type      string `json:"type" yaml:"type"`
	Stack     string `json:"stack" yaml:"stack"`
	Color     string `json:"color" yaml:"color"`
	Link      string `json:"link" yaml:"link"`
	Transform string `json:"transform" yaml:"transform"`
	Yaxis     string `json:"yaxis" yaml:"yaxis"`

	Selectors []GraphSelectors `json:"selectors" yaml:"selectors"`
}

type GraphConfig struct {
	ID        string `json:"id" yaml:"id"`
	ProjectID string `json:"projectId" yaml:"projectId"`

	Name        string `json:"name" yaml:"name"`
	Description string `json:"description" yaml:"description"`

	Parameters []GraphParameters `json:"parameters" yaml:"parameters"`
	Elements   []GraphElements   `json:"elements" yaml:"elements"`

	Aggr                string `json:"aggr" yaml:"aggr"`
	BucketLabel         string `json:"bucketLabel" yaml:"bucketLabel"`
	ColorScheme         string `json:"colorScheme" yaml:"colorScheme"`
	Downsampling        string `json:"downsampling" yaml:"downsampling"`
	DownsamplingAggr    string `json:"downsamplingAggr" yaml:"downsamplingAggr"`
	DownsamplingFill    string `json:"downsamplingFill" yaml:"downsamplingFill"`
	Filter              string `json:"filter" yaml:"filter"`
	FilterBy            string `json:"filterBy" yaml:"filterBy"`
	FilterLimit         string `json:"filterLimit" yaml:"filterLimit"`
	GraphMode           string `json:"graphMode" yaml:"graphMode"`
	Green               string `json:"green" yaml:"green"`
	GreenValue          string `json:"greenValue" yaml:"greenValue"`
	Grid                string `json:"grid" yaml:"grid"`
	HideNoData          bool   `json:"hideNoData" yaml:"hideNoData"`
	IgnoreInf           bool   `json:"ignoreInf" yaml:"ignoreInf"`
	IgnoreMinStepMillis bool   `json:"ignoreMinStepMillis" yaml:"ignoreMinStepMillis"`
	Interpolate         string `json:"interpolate" yaml:"interpolate"`
	Limit               string `json:"limit" yaml:"limit"`
	Max                 string `json:"max" yaml:"max"`
	MaxPoints           int    `json:"maxPoints" yaml:"maxPoints"`
	Min                 string `json:"min" yaml:"min"`
	MovingPercentile    string `json:"movingPercentile" yaml:"movingPercentile"`
	MovingWindow        string `json:"movingWindow" yaml:"movingWindow"`
	Normalize           bool   `json:"normalize" yaml:"normalize"`
	NumberFormat        string `json:"numberFormat" yaml:"numberFormat"`
	OverLinesTransform  string `json:"overLinesTransform" yaml:"overLinesTransform"`
	Percentiles         string `json:"percentiles" yaml:"percentiles"`
	Red                 string `json:"red" yaml:"red"`
	RedValue            string `json:"redValue" yaml:"redValue"`
	Scale               string `json:"scale" yaml:"scale"`
	SecondaryGraphMode  string `json:"secondaryGraphMode" yaml:"secondaryGraphMode"`
	Transform           string `json:"transform" yaml:"transform"`
	Violet              string `json:"violet" yaml:"violet"`
	VioletValue         string `json:"violetValue" yaml:"violetValue"`
	Yellow              string `json:"yellow" yaml:"yellow"`
	YellowValue         string `json:"yellowValue" yaml:"yellowValue"`
}

type GraphsConfigs struct {
	Graphs []GraphConfig `yaml:"graphs"`
}

////////////////////////////////////////////////////////////////////////////////

func loadGraphsConfigsFromFile(filePath string) ([]GraphConfig, error) {
	configData, err := ioutil.ReadFile(filePath)
	if err != nil {
		return nil, err
	}

	graphsConfigs := &GraphsConfigs{}
	if err := yaml.Unmarshal(configData, graphsConfigs); err != nil {
		return nil, fmt.Errorf("%v parsing failed: %v", filePath, err)
	}

	return graphsConfigs.Graphs, nil
}

func loadGraphsConfigsFromPath(path string) ([]GraphConfig, error) {
	files, err := listFiles(path)
	if err != nil {
		return nil, err
	}

	var configs []GraphConfig

	for _, file := range files {
		graphConfigs, err := loadGraphsConfigsFromFile(file)
		if err != nil {
			return nil, err
		}

		configs = append(configs, graphConfigs...)
	}

	return configs, nil
}

////////////////////////////////////////////////////////////////////////////////

func createSolomonGraph(
	graphConfig GraphConfig,
	clusterConfig *ClusterConfig,
	solomonProjectID string,
) (solomon.Graph, error) {

	var graph solomon.Graph

	bytes, err := json.Marshal(graphConfig)
	if err != nil {
		return graph, err
	}

	err = json.Unmarshal(bytes, &graph)
	if err != nil {
		return graph, err
	}

	return graph, nil
}

func createSolomonGraphs(
	graphConfigs []GraphConfig,
	serviceConfig *ServiceConfig,
	opts *Options,
) ([]solomon.Graph, error) {

	projectID := serviceConfig.SolomonProjectID

	graphIds := make(map[string]bool)
	var graphs []solomon.Graph

	for _, graphConfig := range graphConfigs {
		graph, err := createSolomonGraph(graphConfig, serviceConfig.defaultClusterConfig(), projectID)
		if err != nil {
			return nil, err
		}

		if _, ok := graphIds[graph.ID]; ok {
			return nil, fmt.Errorf("multiple graphs with the same ID: %v", graph.ID)
		}

		graphIds[graph.ID] = false
		graphs = append(graphs, graph)
	}

	return graphs, nil
}

func loadSolomonGraphIds(
	ctx context.Context,
	solomonClient solomon.SolomonClientIface,
) (map[string]bool, error) {

	solomonGraphIds, err := solomonClient.ListGraphs(ctx)
	if err != nil {
		return nil, err
	}

	solomonGraphs := make(map[string]bool)

	for _, graphID := range solomonGraphIds {
		solomonGraphs[graphID] = false
	}

	return solomonGraphs, nil
}

////////////////////////////////////////////////////////////////////////////////

type GraphManager struct {
	opts   *Options
	config *ServiceConfig

	solomonClient solomon.SolomonClientIface
}

func (a *GraphManager) Run(ctx context.Context) error {
	graphConfigs, err := loadGraphsConfigsFromPath(a.opts.GraphsPath)
	if err != nil {
		return err
	}

	graphs, err := createSolomonGraphs(graphConfigs, a.config, a.opts)
	if err != nil {
		return err
	}

	solomonGraphIds, err := loadSolomonGraphIds(ctx, a.solomonClient)
	if err != nil {
		return err
	}

	addedGraphCount := 0
	changedGraphCount := 0
	unchangedGraphCount := 0
	for _, graph := range graphs {
		if _, ok := solomonGraphIds[graph.ID]; ok {
			solomonGraphIds[graph.ID] = true

			solomonGraph, err := a.solomonClient.GetGraph(ctx, graph.ID)
			if err != nil {
				return err
			}

			graph.Version = solomonGraph.Version

			if reflect.DeepEqual(solomonGraph, graph) {
				unchangedGraphCount++
			} else {
				changedGraphCount++
				if a.opts.ApplyChanges {
					fmt.Println("Update graph: ", graph.ID)
					_, err := a.solomonClient.UpdateGraph(ctx, graph)
					if err != nil {
						return err
					}
				} else {
					fmt.Println("Need to update graph: ", graph.ID)
				}

				if a.opts.ShowDiff {
					oldGraph, err := json.MarshalIndent(solomonGraph, "", "  ")
					if err != nil {
						return err
					}

					newGraph, err := json.MarshalIndent(graph, "", "  ")
					if err != nil {
						return err
					}

					err = printDiff(graph.ID, oldGraph, newGraph)
					if err != nil {
						return err
					}
				}
			}
		} else {
			addedGraphCount++
			if a.opts.ApplyChanges {
				fmt.Println("Add graph: ", graph.ID)
				_, err := a.solomonClient.AddGraph(ctx, graph)
				if err != nil {
					return err
				}
			} else {
				fmt.Println("Need to add graph: ", graph.ID)
			}
		}
	}

	var untrackedGraphIds []string
	for graphID, tracked := range solomonGraphIds {
		if tracked {
			continue
		}

		untrackedGraphIds = append(untrackedGraphIds, graphID)
	}

	if a.opts.Remove {
		sort.Strings(untrackedGraphIds)
		for _, graphID := range untrackedGraphIds {
			if a.opts.ApplyChanges {
				fmt.Println("Delete graph: ", graphID)
				err = a.solomonClient.DeleteGraph(ctx, graphID)
				if err != nil {
					return err
				}
			} else {
				fmt.Println("Need to delete graph: ", graphID)
			}
		}
	}

	if addedGraphCount != 0 {
		fmt.Println(colorGreen+"Added graphs count:", addedGraphCount, colorReset)
	}

	if changedGraphCount != 0 {
		fmt.Println(colorYellow+"Changed graphs count:", changedGraphCount, colorReset)
	}

	fmt.Println("Unchanged graphs count:", unchangedGraphCount)

	if !a.opts.Remove && len(untrackedGraphIds) != 0 {
		fmt.Printf(colorRed+"Untracked graphs count:%d, %v\n"+colorReset, len(untrackedGraphIds), untrackedGraphIds)
	}
	return nil
}

////////////////////////////////////////////////////////////////////////////////

func NewGraphManager(
	opts *Options,
	config *ServiceConfig,
	solomonClient solomon.SolomonClientIface,
) *GraphManager {

	return &GraphManager{
		opts:          opts,
		config:        config,
		solomonClient: solomonClient,
	}
}
