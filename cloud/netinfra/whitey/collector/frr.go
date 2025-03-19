package collector

import (
	"encoding/json"
	"fmt"
	"os/exec"
	"strings"
	"time"

	"whitey/bgp"
)

// Unmarsal structs
type FrrBGPPeer struct {
	Group  string                     `json:"peerGroup"`
	State  bgp.State                  `json:"bgpState"`
	Uptime uint64                     `json:"bgpTimerUpMsec"`
	AFInfo map[string]json.RawMessage `json:"addressFamilyInfo"`
}

type FrrAFData struct {
	PrefixIn  uint `json:"acceptedPrefixCounter"`
	PrefixOut uint `json:"sentPrefixCounter"`
}

var FrrVrfMap = map[string]string{
	"default": "global",
}

// Collector definition
type FRRCollector struct {
	Interval time.Duration
	Data     Data
	Tasks    []Task
}

func (fc *FRRCollector) GetInterval() time.Duration {
	return fc.Interval
}

func (fc *FRRCollector) SetInterval(interval time.Duration) {
	fc.Interval = interval
}

func (fc *FRRCollector) GetTasks() []Task {
	return fc.Tasks
}

func (fc *FRRCollector) AddTask(t Task) {
	fc.Tasks = append(fc.Tasks, t)
}

func (fc *FRRCollector) Collect() error {
	result, err := exec.Command("vtysh", "-c", "show bgp vrf all neighbors json").Output()
	if err != nil {
		return fmt.Errorf("failed to execute vtysh command: %w", err)
	}

	var jBgpVrfs map[string]map[string]json.RawMessage
	if err := json.Unmarshal(result, &jBgpVrfs); err != nil {
		return fmt.Errorf("failed to unmarshal vtysh output: %w", err)
	}

	for jVrfName, jVrfData := range jBgpVrfs {
		vrfName := jVrfName
		if normalizedVrfName, ok := FrrVrfMap[jVrfName]; ok {
			vrfName = normalizedVrfName
		}

		for jPeerName, jPeerData := range jVrfData {
			if jPeerName == "vrfId" || jPeerName == "vrfName" {
				continue
			}

			frrPeer := &FrrBGPPeer{}
			json.Unmarshal(jPeerData, frrPeer)

			afs := []bgp.AFInfo{}
			for afiName, afiData := range frrPeer.AFInfo {
				frrAFData := &FrrAFData{}
				json.Unmarshal(afiData, &frrAFData)

				afs = append(afs, bgp.AFInfo{
					Name: strings.ToLower(afiName),
					Prefix: &bgp.Prefix{
						In:  frrAFData.PrefixIn,
						Out: frrAFData.PrefixOut,
					},
				})
			}

			peer := bgp.Peer{
				Vrf:     strings.ToLower(vrfName),
				Group:   strings.ToLower(frrPeer.Group),
				Address: jPeerName,
				State:   frrPeer.State,
				Uptime:  frrPeer.Uptime / 1000,
				AFs:     afs,
			}

			fc.Data.BGPPeers = append(fc.Data.BGPPeers, peer)
		}
	}

	return nil
}

func (fc *FRRCollector) GetData() Data {
	return fc.Data
}

func (fc *FRRCollector) Drop() {
	fc.Data = Data{}
}
