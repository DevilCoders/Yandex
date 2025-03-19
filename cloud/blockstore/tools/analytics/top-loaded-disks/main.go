package main

import (
	"bufio"
	"bytes"
	"container/heap"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"strings"
	"time"

	nbs_client "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	coreprotos "a.yandex-team.ru/cloud/storage/core/protos"

	"github.com/spf13/cobra"
)

const (
	// ndaUrl = "https://nda.ya.ru"
	solomonURL   = "http://solomon.yandex.net/api/v2"
	conductorURL = "https://c.yandex-team.ru"
)

type options struct {
	ClusterName string
	DataCenter  string
	OAuth       string
	N           uint32
	DiskType    string
	DiskFile    string
}

type diskStats struct {
	diskID    string
	loadValue float64
}

type StatsHeap []*diskStats

func (h StatsHeap) Len() int           { return len(h) }
func (h StatsHeap) Less(i, j int) bool { return -h[i].loadValue < -h[j].loadValue }
func (h StatsHeap) Swap(i, j int)      { h[i], h[j] = h[j], h[i] }

func (h *StatsHeap) Push(x interface{}) {
	*h = append(*h, x.(*diskStats))
}

func (h *StatsHeap) Pop() interface{} {
	old := *h
	n := len(old)
	x := old[n-1]
	*h = old[0 : n-1]
	return x
}

func readDataFromURL(url string) (string, error) {
	resp, err := http.Get(url)
	if err != nil {
		return "", err
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return "", err
	}

	return string(body), nil
}

/*
func getShortUrl(url string) (string, error) {
    return readDataFromURL(fmt.Sprintf("%s/--?url=%s", ndaUrl, url))
}*/

func getHosts(group string) ([]string, error) {
	resp, err := readDataFromURL(
		fmt.Sprintf(
			"%s/api/groups2hosts/%s",
			conductorURL,
			group,
		))

	if err != nil {
		return nil, err
	}

	lines := strings.Split(resp, "\n")

	endpoints := make([]string, len(lines))

	for i, line := range lines {
		endpoints[i] = line + ":9766"
	}

	return endpoints, nil
}

func newStderrLog() nbs_client.Log {
	return nbs_client.NewLog(
		log.New(os.Stderr, "", log.Ltime),
		nbs_client.LOG_ERROR,
	)
}

func newNbsClient(endpoints []string) (*nbs_client.DiscoveryClient, error) {
	nbs, err := nbs_client.NewDiscoveryClient(
		endpoints,
		&nbs_client.GrpcClientOpts{},
		&nbs_client.DurableClientOpts{},
		&nbs_client.DiscoveryClientOpts{},
		newStderrLog(),
	)

	return nbs, err
}

func getCtrlConductorGroup(cluster string, dc string) string {
	if cluster == "hw-nbs-stable-lab" {
		return "cloud_hw-nbs-stable-lab_nbs-control"
	}

	if cluster == "hw-nbs-dev-lab" {
		return "cloud_hw-nbs-dev-lab_nbs"
	}

	return fmt.Sprintf("cloud_%s_nbs-control_%s", cluster, dc)
}

func checkDiskType(kind coreprotos.EStorageMediaKind, dt string) bool {
	if dt == "nonrepl" {
		return kind == coreprotos.EStorageMediaKind_STORAGE_MEDIA_SSD_NONREPLICATED
	}

	if dt == "mirror2" {
		return kind == coreprotos.EStorageMediaKind_STORAGE_MEDIA_SSD_MIRROR2
	}

	if dt == "mirror3" {
		return kind == coreprotos.EStorageMediaKind_STORAGE_MEDIA_SSD_MIRROR3
	}

	if dt == "local" {
		return kind == coreprotos.EStorageMediaKind_STORAGE_MEDIA_SSD_LOCAL
	}

	if dt == "ssd" {
		return kind == coreprotos.EStorageMediaKind_STORAGE_MEDIA_SSD
	}

	if dt == "hdd" {
		return kind == coreprotos.EStorageMediaKind_STORAGE_MEDIA_HDD
	}

	if dt == "hybrid" {
		return kind == coreprotos.EStorageMediaKind_STORAGE_MEDIA_HYBRID
	}

	return false
}

func collectDisksFromNBS(ctx context.Context, opts *options) ([]string, error) {
	ls, err := getHosts(getCtrlConductorGroup(opts.ClusterName, opts.DataCenter))
	if err != nil {
		return nil, err
	}

	nbs, err := newNbsClient(ls)
	if err != nil {
		return nil, err
	}

	volumes, err := nbs.ListVolumes(ctx)
	if err != nil {
		return nil, err
	}

	if opts.DiskType == "" {
		return volumes, nil
	}

	ch := make(chan string)

	for _, diskID := range volumes {
		go func(diskID string) {
			vol, err := nbs.DescribeVolume(ctx, diskID)
			if err != nil || !checkDiskType(vol.StorageMediaKind, opts.DiskType) {
				ch <- ""
			} else {
				ch <- diskID
			}
		}(diskID)
	}

	result := []string{}

	for i := 0; i < len(volumes); i++ {
		s := <-ch

		if s != "" {
			result = append(result, s)
		}
	}

	return result, nil
}

func collectDisksFromFile(ctx context.Context, opts *options) ([]string, error) {
	file, err := os.Open(opts.DiskFile)
	if err != nil {
		return nil, err
	}

	scanner := bufio.NewScanner(file)
	scanner.Split(bufio.ScanLines)

	result := []string{}

	for scanner.Scan() {
		result = append(result, scanner.Text())
	}

	return result, nil
}

func collectDisks(ctx context.Context, opts *options) (*StatsHeap, error) {
	var volumes []string
	var err error

	if opts.DiskFile != "" {
		volumes, err = collectDisksFromFile(ctx, opts)
	} else {
		volumes, err = collectDisksFromNBS(ctx, opts)
	}

	if err != nil {
		return nil, err
	}

	ch := make(chan *diskStats)

	for _, diskID := range volumes {
		go func(diskID string) {
			v, err := getLoadValue(ctx, diskID, opts)
			if err != nil {
				ch <- &diskStats{}
			} else {
				ch <- &diskStats{
					diskID:    diskID,
					loadValue: v,
				}
			}
		}(diskID)
	}

	result := &StatsHeap{}

	for i := 0; i < len(volumes); i++ {
		s := <-ch

		if s.diskID != "" {
			heap.Push(result, s)
		}
	}

	return result, nil
}

type solomonParams struct {
	Program string `json:"program"`
	From    string `json:"from"`
	To      string `json:"to"`
}

type solomonResponse struct {
	Scalar float64 `json:"scalar"`
}

func getSolomonClusterName(clusterName string) string {
	if strings.HasPrefix(clusterName, "hw-") {
		return "cloud_" + clusterName
	}
	return "yandexcloud_" + clusterName
}

func getSolomonHost(dc string) string {
	return "cluster"
}

func getLoadValue(ctx context.Context, diskID string, opts *options) (float64, error) {
	expression := fmt.Sprintf(`
        let total_requests = integrate_fn(group_lines('sum', {
            cluster="%s",
            service="server_volume",
            host="%s",
            sensor="Count",
            request="*",
            volume="%s"
            }));

            return max(total_requests);`,
		getSolomonClusterName(opts.ClusterName),
		getSolomonHost(opts.DataCenter),
		diskID,
	)

	t := time.Now()
	params := &solomonParams{
		From:    t.Add(-time.Hour * 24).Format(time.RFC3339),
		To:      t.Format(time.RFC3339),
		Program: expression}

	payload, _ := json.Marshal(params)

	req, err := http.NewRequest(
		"POST",
		"https://solomon.yandex-team.ru/api/v2/projects/nbs/sensors/data",
		bytes.NewBuffer(payload),
	)
	if err != nil {
		return 0.0, err
	}

	req.Header.Add("Authorization", "OAuth "+opts.OAuth)
	req.Header.Add("Content-Type", "application/json;charset=UTF-8")
	req.Header.Add("Accept", "application/json")

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return 0.0, err
	}

	defer resp.Body.Close()
	body, err := ioutil.ReadAll(resp.Body)
	if err != nil {
		return 0.0, err
	}

	r := &solomonResponse{}

	if err := json.Unmarshal(body, r); err != nil {
		return 0.0, err
	}

	return r.Scalar, nil
}

func run(ctx context.Context, opts *options) error {
	stats, err := collectDisks(ctx, opts)
	if err != nil {
		return err
	}

	for i := uint32(0); i < opts.N; i++ {
		if stats.Len() == 0 {
			break
		}
		s := heap.Pop(stats).(*diskStats)
		fmt.Printf("%s: %f\n", s.diskID, s.loadValue)
	}

	return nil
}

func main() {
	var opts options

	var rootCmd = &cobra.Command{
		Use:   "blockstore-top-disks",
		Short: "Top most loaded disks",
		Run: func(cmd *cobra.Command, args []string) {
			ctx, cancelCtx := context.WithCancel(context.Background())
			defer cancelCtx()

			if err := run(ctx, &opts); err != nil {
				log.Fatalf("Error: %v", err)
			}
		},
	}

	rootCmd.Flags().Uint32Var(
		&opts.N,
		"top",
		10,
		"report top N disks",
	)

	rootCmd.Flags().StringVar(
		&opts.DiskFile,
		"disks-file",
		"",
		"file with disks",
	)

	rootCmd.Flags().StringVar(
		&opts.ClusterName,
		"cluster",
		"",
		"cluster name (prod, preprod)",
	)

	rootCmd.Flags().StringVar(
		&opts.DataCenter,
		"dc",
		"",
		"datacenter (sas, vla, myt, etc)",
	)

	rootCmd.Flags().StringVar(
		&opts.OAuth,
		"oauth",
		"",
		"OAuth token for Solomon",
	)

	rootCmd.Flags().StringVar(
		&opts.DiskType,
		"disk-type",
		"",
		"disk type (nonrepl, hdd, ssd, hybrid)",
	)

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("Error: %v", err)
	}
}
