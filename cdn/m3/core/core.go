package core

// Main module implements generic functions ...

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"net"
	"net/http"
	"os"
	"sort"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/ptypes"
	"github.com/miekg/dns"
	api "github.com/osrg/gobgp/api"
	gobgp "github.com/osrg/gobgp/pkg/server"
	log "github.com/sirupsen/logrus"

	"a.yandex-team.ru/cdn/m3/cmd/config"
	"a.yandex-team.ru/cdn/m3/lxd"
	"a.yandex-team.ru/cdn/m3/monitor"
	"a.yandex-team.ru/cdn/m3/utils"
)

type Core struct {
	g *config.CmdGlobal

	httpapi *Api

	// map for paths
	paths TMapPath

	// server instance to query from different routines
	// initilized in bgpStartSession, used in ... TODO
	servers map[string]*gobgp.BgpServer

	// bgp notifies channel
	notifies chan Notify
}

// Create core object
func CreateCore(g *config.CmdGlobal) (*Core, error) {
	var err error

	var core Core
	core.g = g
	core.servers = make(map[string]*gobgp.BgpServer)
	core.paths.counters = make(map[string]int)
	core.paths.neigbors = make(map[string][]string)

	return &core, err
}

type BgpOptionsOverrides struct {
	Timeout int    `json:"timeout"`
	Safi    string `json:"safi"`
}

func (b *BgpOptionsOverrides) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("timeout:'%d'", b.Timeout))
	out = append(out, fmt.Sprintf("safi:'%s'", b.Safi))

	return fmt.Sprintf("%s", strings.Join(out, ", "))
}

const (
	BGPOPTIONS_TIMEOUT      = 1
	BGPOPTIONS_STARTFOREVER = -1
)

// notify and result processing data structures
// we have got *Path structure
type Notify struct {
	path *Path
}

type Result struct {
	err     error
	success bool
}

// a bgp notify event worker processor, could be triggered
// from bgp notitication processing or from periodically
// syncronization (1/minute)
func (c *Core) bgpNofifyWorker(id int, notifies <-chan Notify,
	results chan<- Result) {

	jid := "(collector)"
	for j := range notifies {

		// Need some timeout (as we could have a list of
		// events at startup
		time.Sleep(10 * time.Millisecond)

		paths := "['*']"
		if j.path != nil {
			paths = j.path.AsString()
		}

		c.g.Log.Debug(fmt.Sprintf("%s %s nid:'%02d' path: %s",
			utils.GetGPid(), jid, id, paths))

		// Detecting current startup time in seconds
		wait := c.g.Opts.Network.StartupWaittime

		age := float64(time.Since(c.g.Opts.Runtime.T0).Seconds())
		c.g.Log.Debug(fmt.Sprintf("%s %s nid:'%02d' age:'%2.2f' wait:'%d'",
			utils.GetGPid(), jid, id, age, wait))

		if age < float64(wait) {
			err := errors.New(fmt.Sprintf("waiting for startup time"))
			c.g.Log.Error(fmt.Sprintf("%s, skipping process, err:'%s'", jid, err))
			results <- Result{success: false, err: err}
			continue
		}

		var SyncOptionsOverrides SyncOptionsOverrides
		SyncOptionsOverrides.Dryrun = false
		SyncOptionsOverrides.BgpStart = false
		SyncOptionsOverrides.Timeout = BGPOPTIONS_STARTFOREVER

		var err error
		if err = c.networkSync(&SyncOptionsOverrides); err != nil {
			c.g.Log.Error(fmt.Sprintf("error network sync, err:'%s'", err))
			results <- Result{success: false, err: err}
			continue
		}

		results <- Result{success: true}
	}
}

// a go routine to wait for some bgp events
func (c *Core) bgpNofifyCollector() error {
	var err error
	id := "(notify)"

	buffer := 10
	c.notifies = make(chan Notify, buffer)
	result := make(chan Result, buffer)
	workers := 1

	c.g.Log.Debug(fmt.Sprintf("%s %s collector started buffer:'%d' workers:'%d'",
		utils.GetGPid(), id, buffer, workers))

	for w := 1; w <= workers; w++ {
		go c.bgpNofifyWorker(w, c.notifies, result)
	}

	fails := 0
	for j := 1; ; j++ {
		p := <-result
		if !p.success {
			fails++
			c.g.Log.Error(fmt.Sprintf("%s error jid:[%d] counts:[%d] processing, err:'%s'",
				utils.GetGPid(), j, fails, p.err))
			continue
		}

		c.g.Log.Debug(fmt.Sprintf("%s %s collector [%d]/[%d] notify processed",
			utils.GetGPid(), id, j, buffer))
	}
	close(result)

	c.g.Log.Debug(fmt.Sprintf("%s %s collector finished",
		utils.GetGPid(), id))

	return err
}

func (c *Core) getSafis() map[string][]string {
	id := "(safis)"
	// we need N bgp sessions each for safi family, first
	// detecting a list of different safi families in peers
	bgp := c.g.Opts.Bgp
	safis := make(map[string][]string)
	for _, p := range bgp.Peers {
		// peer could have two notations: (1) ip and
		// (2) SAFI@ip, e.g. "MPLS_VPN@2a02:6b8:0:370f::77"
		safi := bgp.DefaultSafi
		peer := p
		if strings.Contains(p, "@") {
			tags := strings.Split(p, "@")
			if len(tags) > 0 {
				safi = tags[0]
				peer = tags[1]
			}
		}
		safis[safi] = append(safis[safi], peer)
	}

	// Dumping generated configuration
	i := 0
	for s, p := range safis {
		i++
		c.g.Log.Debug(fmt.Sprintf("%s %s config bgp [%d]/[%d] SAFI:'%s' peers:[%d]:['%s']",
			utils.GetGPid(), id, i, len(safis), s, len(p),
			strings.Join(p, ",")))
	}

	return safis
}

func (c *Core) getPeersBySafi(safi string) []string {
	peers := c.getSafis()
	if _, ok := peers[safi]; ok {
		return peers[safi]
	}
	return []string{}
}

func (c *Core) bgpStartSession(BgpOptionsOverrides *BgpOptionsOverrides) error {
	var err error

	id := "(core)"

	bgp := c.g.Opts.Bgp
	c.g.Log.Debug(fmt.Sprintf("%s %s bgp session config %s",
		utils.GetGPid(), id, bgp.AsString()))

	// Bgp session should be run with Safi
	// defined below, getting all peers with Safi
	Safi := config.DEFAULT_SAFI
	if BgpOptionsOverrides != nil {
		Safi = BgpOptionsOverrides.Safi
		c.g.Log.Debug(fmt.Sprintf("%s %s bgp overrides %s",
			utils.GetGPid(), id, BgpOptionsOverrides.AsString()))
	}

	// setting log for gobgp library
	if c.g.Log.LogType == config.LOGTYPE_FILE {
		log.SetOutput(c.g.Log.File)
		log.SetLevel(log.DebugLevel)
	}

	c.servers[Safi] = gobgp.NewBgpServer()
	go c.servers[Safi].Serve()

	var global api.Global
	global.As = bgp.As
	global.RouterId = bgp.RouterId
	global.ListenPort = bgp.LocalPort
	global.UseMultiplePaths = true

	if err = c.servers[Safi].StartBgp(context.Background(),
		&api.StartBgpRequest{Global: &global}); err != nil {

		c.g.Log.Error(fmt.Sprintf("error starting bgp session, err:'%s'", err))
		return err
	}

	c.g.Log.Debug(fmt.Sprintf("%s %s bgp session started", utils.GetGPid(), id))

	if err = c.servers[Safi].MonitorPeer(context.Background(), &api.MonitorPeerRequest{},
		func(p *api.Peer) {
			c.g.Log.Debug(fmt.Sprintf("%s %s peer %s", utils.GetGPid(), id, p))
		}); err != nil {

		c.g.Log.Error(fmt.Sprintf("error monitoring peer, err:'%s'", err))
		return err
	}

	ipv6MPLS := &api.Family{}
	ipv4MPLS := &api.Family{}

	switch Safi {
	case "MPLS_LABEL":
		ipv6MPLS = &api.Family{
			Afi:  api.Family_AFI_IP6,
			Safi: api.Family_SAFI_MPLS_LABEL,
		}

		ipv4MPLS = &api.Family{
			Afi:  api.Family_AFI_IP,
			Safi: api.Family_SAFI_MPLS_LABEL,
		}

	case "MPLS_VPN":
		ipv6MPLS = &api.Family{
			Afi:  api.Family_AFI_IP6,
			Safi: api.Family_SAFI_MPLS_VPN,
		}

		ipv4MPLS = &api.Family{
			Afi:  api.Family_AFI_IP,
			Safi: api.Family_SAFI_MPLS_VPN,
		}
	}

	count := 1
	afiSafis := make([]*api.AfiSafi, 0, count)
	afiSafi6 := &api.AfiSafi{
		Config: &api.AfiSafiConfig{
			Family:  ipv6MPLS,
			Enabled: true,
		},
		AddPaths: &api.AddPaths{
			Config: &api.AddPathsConfig{
				Receive: true,
				SendMax: 32,
			},
		},
	}

	afiSafi4 := &api.AfiSafi{
		Config: &api.AfiSafiConfig{
			Family:  ipv4MPLS,
			Enabled: true,
		},
		AddPaths: &api.AddPaths{
			Config: &api.AddPathsConfig{
				Receive: true,
				SendMax: 32,
			},
		},
	}

	afiSafis = append(afiSafis, afiSafi6)
	afiSafis = append(afiSafis, afiSafi4)

	// Need construct peers of Safi defined
	peers := c.getPeersBySafi(Safi)
	c.g.Log.Debug(fmt.Sprintf("%s %s safi:'%s' bgp peers:['%s']",
		utils.GetGPid(), id, Safi,
		strings.Join(peers, ",")))

	var T api.Transport
	T.LocalPort = uint32(bgp.LocalPort)
	T.RemotePort = uint32(bgp.RemotePort)
	T.PassiveMode = bgp.Passive

	if len(bgp.LocalAddress) > 0 {
		T.LocalAddress = bgp.LocalAddress
	}

	for _, p := range peers {

		/*
			slayer@build-bionic-01i:~/go/src/github.com/osrg/gobgp$ diff -u -u pkg/server/fsm.go.orig pkg/server/fsm.go
			/home/slayer/go/src/github.com/osrg/gobgp/pkg/server/fsm.go

			--- pkg/server/fsm.go.orig	2020-11-13 18:38:48.026562792 +0300
			+++ pkg/server/fsm.go	2020-11-13 18:39:06.206705922 +0300
			@@ -760,7 +760,6 @@
			 	}

			 	// unnumbered BGP
			-	if pConf.Config.NeighborInterface != "" {
			 		tuples := []*bgp.CapExtendedNexthopTuple{}
			 		families, _ := config.AfiSafis(pConf.AfiSafis).ToRfList()
			 		for _, family := range families {
			@@ -771,7 +770,6 @@
			 			tuples = append(tuples, tuple)
			 		}
			 		caps = append(caps, bgp.NewCapExtendedNexthop(tuples))
			-	}

			 	// ADD-PATH Capability
			 	if c := capAddPathFromConfig(pConf); c != nil {
		*/

		n := &api.Peer{
			Conf: &api.PeerConf{
				NeighborAddress: p,
				PeerAs:          bgp.PeerAs,
				LocalAs:         bgp.As,
			},
			Transport: &T,
			EbgpMultihop: &api.EbgpMultihop{
				Enabled:     true,
				MultihopTtl: 64,
			},
			GracefulRestart: &api.GracefulRestart{
				Enabled:          true,
				LonglivedEnabled: true,
			},
			AfiSafis: afiSafis,
		}

		if err := c.servers[Safi].AddPeer(context.Background(), &api.AddPeerRequest{
			Peer: n}); err != nil {

			c.g.Log.Error(fmt.Sprintf("error adding peer, err:'%s'", err))
		}
	}

	marshaller := jsonpb.Marshaler{
		Indent:   "  ",
		OrigName: true,
	}

	// Session could be start forerver (if
	// timeout set to core.BGPOPTIONS_STARTFOREVER
	var timeout int
	timeout = BGPOPTIONS_TIMEOUT
	if BgpOptionsOverrides != nil {
		timeout = BgpOptionsOverrides.Timeout
	}

	// Display incoming Prefixes in JSON format.
	if err := c.servers[Safi].MonitorTable(context.Background(), &api.MonitorTableRequest{
		TableType: api.TableType_GLOBAL,
		/*
		   Family: &api.Family{
		           //Afi:  api.Family_AFI_IP6,
		           Afi:  api.Family_AFI_IP,
		           Safi: api.Family_SAFI_MPLS_LABEL,
		   },
		*/
	}, func(p *api.Path) {

		// Some path arrived, we need (1) parse it and update
		// internal 2-version map (2) notify a processing
		// task routine (it waits on notification channel)

		if c.g.Log.LogType == config.LOGTYPE_FILE {
			marshaller.Marshal(c.g.Log.File, p)
		}
		if c.g.Log.LogType == config.LOGTYPE_STDOUT {
			marshaller.Marshal(os.Stdout, p)
		}

		var err error
		var path *Path
		if path, err = c.bgpPathParse(p); err != nil {
			c.g.Log.Error(fmt.Sprintf("error parsing path, err:'%s'", err))
			return
		}

		if path == nil {
			// path is filtered
			return
		}

		c.g.Log.Debug(fmt.Sprintf("%s %s path parsed '%s'",
			utils.GetGPid(), id, path.AsString()))

		path.DumpAsJson(c.g)

		if err = c.paths.Update(c.g, path); err != nil {
			c.g.Log.Error(fmt.Sprintf("error updating path, err:'%s'", err))
			return
		}

		c.notifies <- Notify{path: path}

	}); err != nil {
		c.g.Log.Error(fmt.Sprintf("error monitoring table, err:'%s'", err))
	}

	if timeout > 0 {
		minutes, _ := time.ParseDuration(fmt.Sprintf("%ds", timeout))
		time.Sleep(minutes)
	}

	// TODO: c.servers[Safi].DeletePeer()

	return err
}

func ExtractCommunity(c uint32) Community {
	high := (0xffff0000 & c) >> 16
	low := 0xffff & c
	C := Community{High: high, Low: low}
	return C
}

// Parsing api.Path object to extract attributes
// and their values to generate a paths map
func (c *Core) bgpPathParse(p *api.Path) (*Path, error) {
	var err error
	var path Path

	id := "(parse)"

	// Parsing family as family.Afi, family.Safi
	family := p.GetFamily()
	path.Afi = family.Afi
	path.Safi = family.Safi

	// getting neighbor ip
	neighbor := p.GetNeighborIp()
	path.Neighbor = neighbor

	// getting ids: local id and remote id, assuming
	// that triple id, local-id and neighbor is uniq?
	path.Identifier = p.GetIdentifier()
	path.LocalIdentifier = p.GetLocalIdentifier()

	// Event could be accepted or withdrawn
	path.Event = BGPEVENT_ACCEPTED
	if p.GetIsWithdraw() {
		path.Event = BGPEVENT_WITHDRAW
	}

	path.SourceAsn = p.GetSourceAsn()

	// getting nlri and processing it, please
	// be sure to have here all types below in
	// switch for Message type
	nlri := p.GetNlri()

	var value ptypes.DynamicAny
	if err = ptypes.UnmarshalAny(nlri, &value); err != nil {
		err = errors.New(fmt.Sprintf("parse error, err:'%s'", err))
		return nil, err
	}

	userd := true
	switch v := value.Message.(type) {
	case *api.LabeledIPAddressPrefix:
		// Detecting prefix and its length
		path.Prefix = v.Prefix
		path.PrefixLen = v.PrefixLen

		// Parsing mpls (filtering?)
		for _, l := range v.Labels {
			path.Labels = append(path.Labels, l)
		}
	case *api.LabeledVPNIPAddressPrefix:
		// Detecting prefix and its length
		path.Prefix = v.Prefix
		path.PrefixLen = v.PrefixLen

		// Parsing mpls (filtering?)
		for _, l := range v.Labels {
			path.Labels = append(path.Labels, l)
		}

		var value ptypes.DynamicAny
		if err = ptypes.UnmarshalAny(v.Rd, &value); err != nil {
			return nil, fmt.Errorf("failed to unmarshal route distinguisher: %s", err)
		}

		switch v := value.Message.(type) {
		case *api.RouteDistinguisherIPAddress:
			A := uint32(v.Assigned)
			C := Community{High: 13238, Low: A}

			c.g.Log.Debug(fmt.Sprintf("%s %s rd: admin:'%s' assigned:'%d' -> community:'%s'",
				utils.GetGPid(), id, v.Admin, A, C.AsString()))

			if userd {
				path.Community.Community =
					append(path.Community.Community, C)
			}
		}

	default:
		err = errors.New(fmt.Sprintf("unknown api path type"))
		return nil, err
	}

	// getting attributes: communities and next hops
	pattrs := p.GetPattrs()
	for _, p := range pattrs {
		var value ptypes.DynamicAny
		if err := ptypes.UnmarshalAny(p, &value); err != nil {
			return nil, fmt.Errorf("failed to unmarshal attr: %s", err)
		}

		switch v := value.Message.(type) {
		case *api.CommunitiesAttribute:
			for _, c := range v.Communities {
				if !userd {
					path.Community.Community =
						append(path.Community.Community,
							ExtractCommunity(c))
				}

			}
		case *api.MpReachNLRIAttribute:
			for _, h := range v.NextHops {
				path.NextHops = append(path.NextHops, fmt.Sprintf("%s", h))
			}
		}
	}

	// detecting a linkid from a list of comminity
	path.Community.Linkid = 0
	links := c.g.Opts.Objects.Links
	for _, c := range path.Community.Community {
		if c.High != links.High && c.Low < links.Low {
			continue
		}

		// TODO if there are severl such communities
		path.Community.Linkid = c.Low - links.Low
	}

	if path.Community.Linkid == 0 {
		err = errors.New(fmt.Sprintf("path:'%s' error detecting linkid, community does not match",
			path.AsString()))
		return nil, err
	}

	// Filtering linkid it should be in interval of
	// [links.Match.Min, links.Match.Max]
	if int(path.Community.Linkid) < links.Match.Min ||
		int(path.Community.Linkid) > links.Match.Max {

		c.g.Log.Debug(fmt.Sprintf("%s %s path:'%s' skipping linkid:'%d' as not matched to be in interval of '[%d %d]'",
			utils.GetGPid(), id, path.AsString(), path.Community.Linkid, links.Match.Min,
			links.Match.Max))

		return nil, nil
	}

	// Checking path agains filter (if any)
	c.g.Log.Debug(fmt.Sprintf("%s %s (config) filtering links:'%s'",
		utils.GetGPid(), id, links.AsString()))

	loc := config.LocationAsString(c.g.Opts.Runtime.Location)
	fqdn := c.g.Opts.Runtime.Hostname

	c.g.Log.Debug(fmt.Sprintf("%s %s (attributes) fqdn:'%s' location:'%s'",
		utils.GetGPid(), id, fqdn, loc))

	linkid := path.Community.Linkid

	tags := []string{fqdn, loc}
	for _, t := range tags {
		// Looking up for fqdn
		if v, ok := links.Filter[t]; ok {
			if v.Enabled {
				if !utils.IntInSlice(int(linkid), v.Links) {
					err = errors.New(fmt.Sprintf("linkid:'%d' filtered on '%s' skip:'%s'",
						linkid, v.AsString(), path.AsString()))
					c.g.Log.Error(fmt.Sprintf("%s %s skip path, as skip:'%s'",
						utils.GetGPid(), id, err))
					return nil, nil
				}
			}
		}
	}

	return &path, err
}

func (c *Core) bgpGetPaths() ([]Path, error) {
	var err error
	var out []Path

	id := "(bgp)"

	safis := c.getSafis()

	for safi, _ := range safis {

		if c.servers[safi] == nil {
			err = errors.New("bgp session is not established")
			c.g.Log.Error(fmt.Sprintf("error getting table, err:'%s'", err))
			return out, err
		}

		marshaller := jsonpb.Marshaler{
			Indent:   "  ",
			OrigName: true,
		}

		var families []api.Family

		if safi == "MPLS_LABEL" {
			families = append(families, api.Family{
				Afi:  api.Family_AFI_IP,
				Safi: api.Family_SAFI_MPLS_LABEL,
			})

			families = append(families, api.Family{
				Afi:  api.Family_AFI_IP6,
				Safi: api.Family_SAFI_MPLS_LABEL,
			})
		}

		// Also could be vpn_mpls
		if safi == "MPLS_VPN" {
			families = append(families, api.Family{
				Afi:  api.Family_AFI_IP,
				Safi: api.Family_SAFI_MPLS_VPN,
			})

			families = append(families, api.Family{
				Afi:  api.Family_AFI_IP6,
				Safi: api.Family_SAFI_MPLS_VPN,
			})
		}

		for _, af := range families {

			c.servers[safi].ListPath(context.Background(), &api.ListPathRequest{
				TableType: api.TableType_GLOBAL,
				Family:    &af,
			}, func(d *api.Destination) {

				for _, p := range d.Paths {
					var path *Path
					if path, err = c.bgpPathParse(p); err != nil {
						c.g.Log.Error(fmt.Sprintf("error parsing path, err:'%s'", err))
						return
					}

					if path == nil {
						// it could have place if path is
						// filtered against Filter
						continue
					}

					c.g.Log.Debug(fmt.Sprintf("%s %s (list) path parsed '%s'",
						utils.GetGPid(), id, path.AsString()))

					out = append(out, *path)
				}

				// making m3 less verbose by default, controlled
				// by switch from config bgp.messages-dumped: false
				if c.g.Opts.Bgp.MessagesDumped {
					if c.g.Log.LogType == config.LOGTYPE_FILE {
						marshaller.Marshal(c.g.Log.File, d)
					}
					if c.g.Log.LogType == config.LOGTYPE_STDOUT {
						marshaller.Marshal(os.Stdout, d)
					}
				}
			})
		}
	}

	return out, err
}

// Converting community notation of "high:low"
// represented as string into community number
func StringToCommunity(str string) uint32 {
	tags := strings.Split(str, ":")

	high, _ := strconv.ParseUint(tags[0], 10, 16)
	low, _ := strconv.ParseUint(tags[1], 10, 16)

	return uint32(high<<16 | low)
}

// Convering uint32 community number into
// string representation as "high:low"
func CommunityToString(community uint32) string {

	high := (0xffff0000 & community) >> 16
	low := 0xffff & community

	return fmt.Sprintf("%d:%d", high, low)
}

// Community is stored as high+low uint numbers
// from original uint32 (as we use High and Low
// parts for processing), 867566568 is translated
// in e.g. (13238,1003)

type Community struct {
	High uint32 `json:"high"`
	Low  uint32 `json:"low"`
}

func (c *Community) AsString() string {
	return fmt.Sprintf("(%d:%d)", c.High, c.Low)
}

// Each path for processing should have so called
// "resolved" community to detect a link associated
// with a path. Now rule is High == AS(13238) and
// Low is 10000+linkid, e.g. "13238,10155" resolved
// community and link assosicated has linkd = 155
type ResolvedCommunity struct {
	Linkid    uint32      `json:"linkid"`
	Community []Community `json:"community"`
}

func (c *ResolvedCommunity) AsString() string {

	var out []string

	out = append(out, fmt.Sprintf("linkid:'%d'", c.Linkid))

	for _, c := range c.Community {
		out = append(out, fmt.Sprintf("%s", c.AsString()))
	}

	sort.Strings(out)

	return fmt.Sprintf("%s", strings.Join(out, "; "))
}

const (
	BGPEVENT_WITHDRAW = 1001
	BGPEVENT_ACCEPTED = 1002
	BGPEVENT_UNKNOWN  = 0
)

func bgpEventAsString(event int) string {
	events := map[int]string{
		BGPEVENT_WITHDRAW: "WITHDRAW",
		BGPEVENT_ACCEPTED: "ACCEPTED",
		BGPEVENT_UNKNOWN:  "UNKNOWN",
	}

	if _, ok := events[event]; !ok {
		return events[BGPEVENT_UNKNOWN]
	}
	return events[event]
}

// Path is a parsed bgp api.Path structure. Attributes
// are used to generate a map to sync with ip route, ip link,
// ip rule commands later on
type Path struct {

	// afi and safi in terms of api.path constantd
	// AFI_IP6 AFI_IP and SAFI_MPLS_LABEL
	Afi  api.Family_Afi  `json:"afi"`
	Safi api.Family_Safi `json:"safi"`

	Neighbor string `json:"neighbor"`

	Identifier      uint32 `json:"identifier"`
	LocalIdentifier uint32 `json:"local-identifier"`

	// Event could be one of BGPEVENT_WITHDRAW or
	// BGPEVENT_ACCEPTED, determined as path parsed
	Event int `json:"event"`

	// Source asn
	SourceAsn uint32 `json:"source-asn"`

	// Prefix in path and its length, e.g.
	// for "0.0.0.0/0" and "::0/0" prefixes
	// len is "0"
	Prefix    string `json:"prefix"`
	PrefixLen uint32 `json:"prefix-len"`

	Labels []uint32 `json:"labels"`

	// Resolved community already has a linkid
	// detected from community list
	Community ResolvedCommunity `json:"community"`

	NextHops []string `json:"next-hops"`
}

// Detecting a device tunnel name as md5
// of NextHops, assuming that len of NextHops is 1
const (
	HASHNEXTHOP_LEN = 6
)

func (p *Path) HashNexthop() string {
	hashed := ""
	if len(p.NextHops) == 1 {
		hash := utils.Md5(p.NextHops[0])
		if len(hash) > HASHNEXTHOP_LEN {
			return hash[0 : HASHNEXTHOP_LEN-1]
		}
		return hash
	}
	return hashed
}

// Converting path afi to route fi
func (p *Path) RouteAfi() int {
	afis := map[api.Family_Afi]int{
		api.Family_AFI_IP6: ROUTE_IP6,
		api.Family_AFI_IP:  ROUTE_IP4,
	}

	if _, ok := afis[p.Afi]; !ok {
		return ROUTE_IP4
	}
	return afis[p.Afi]
}

func (p *Path) DumpAsJson(g *config.CmdGlobal) {
	var err error
	var text []byte

	id := "(json)"
	if text, err = json.MarshalIndent(p, "", "\t"); err != nil {
		g.Log.Error(fmt.Sprintf("%s %s (dump) err:'%s'",
			utils.GetGPid(), id, err))
		return
	}

	tags := strings.Split(string(text), "\n")
	for i, r := range tags {
		g.Log.Debug(fmt.Sprintf("%s %s [%d]/[%d] (dump) %s",
			utils.GetGPid(), id, i, len(tags), r))
	}
}

func (p *Path) AsMinimalString() string {
	var out []string
	out = append(out, fmt.Sprintf("'%s'",
		api.Family_Afi_name[int32(p.Afi)]))

	out = append(out, fmt.Sprintf("'%s/%d'",
		p.Prefix, p.PrefixLen))

	var labels []string
	for _, l := range p.Labels {
		labels = append(labels, fmt.Sprintf("%d", l))
	}

	sort.Strings(labels)
	out = append(out, fmt.Sprintf("labels:'[%s]'",
		strings.Join(labels, ";")))

	out = append(out, fmt.Sprintf("linkid:'%d'",
		p.Community.Linkid))

	out = append(out, fmt.Sprintf("next hops:'[%s]'",
		strings.Join(p.NextHops, ", ")))

	return fmt.Sprintf("%s", strings.Join(out, ", "))
}

func (p *Path) AsShortString() string {
	var out []string

	out = append(out, fmt.Sprintf("afi:'%d':'%s'",
		p.Afi, api.Family_Afi_name[int32(p.Afi)]))

	out = append(out, fmt.Sprintf("prefix:'%s', len:'%d'",
		p.Prefix, p.PrefixLen))

	var labels []string
	for _, l := range p.Labels {
		labels = append(labels, fmt.Sprintf("%d", l))
	}

	sort.Strings(labels)

	out = append(out, fmt.Sprintf("labels:'[%s]'",
		strings.Join(labels, ";")))

	var communities []string
	for _, c := range p.Community.Community {
		communities = append(communities,
			fmt.Sprintf("%s", c.AsString()))
	}

	sort.Strings(communities)

	out = append(out, fmt.Sprintf("communities:'[%s]' -> linkid:'%d'",
		strings.Join(communities, ";"), p.Community.Linkid))

	return fmt.Sprintf("%s", strings.Join(out, "; "))
}

func (p *Path) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("safi:'%d':'%s'",
		p.Safi, api.Family_Safi_name[int32(p.Safi)]))

	out = append(out, fmt.Sprintf("neighbor:'%s'",
		p.Neighbor))

	out = append(out, fmt.Sprintf("id:'%d' -> local-id:'%d'",
		p.Identifier, p.LocalIdentifier))

	out = append(out, fmt.Sprintf("event:'%d':'%s'",
		p.Event, bgpEventAsString(p.Event)))

	out = append(out, fmt.Sprintf("source-asn:'%d'",
		p.SourceAsn))

	out = append(out, fmt.Sprintf("next hops:'[%s]'",
		strings.Join(p.NextHops, ", ")))

	return fmt.Sprintf("%s %s", strings.Join(out, "; "),
		p.AsShortString())
}

func (p *Path) Equal(q *Path) bool {
	if q == nil {
		return false
	}
	return p.AsShortString() == q.AsShortString()
}

type TMapPath struct {

	// shared sync map to store RIB a key here
	// is "a prefix"+"linkid community". it should be
	// uniq. A "linkid community" is exaclty one
	// community in from "AS"+"10000+linkid".
	rib sync.Map

	// need counters for paths and mutex
	mutex    sync.RWMutex
	counters map[string]int
	neigbors map[string][]string
}

func RemoveIndex(s []string, index int) []string {
	return append(s[:index], s[index+1:]...)
}

func (t *TMapPath) Update(g *config.CmdGlobal, p *Path) error {
	t.mutex.Lock()
	defer t.mutex.Unlock()

	var err error
	id := "(update)"
	if p == nil {
		err = errors.New("path is empty")
		return err
	}

	// assuming the following uniq key w.r.t different neighbors

	var labels []string
	for _, l := range p.Labels {
		labels = append(labels, fmt.Sprintf("%d", l))
	}

	sort.Strings(labels)

	key := fmt.Sprintf("%d-%d-%s-%d-%s", p.Afi, p.Safi,
		p.Prefix, p.PrefixLen, strings.Join(labels, ";"))

	if p.Event == BGPEVENT_ACCEPTED {

		neigbors := ""
		if !utils.StringInSlice(p.Neighbor, t.neigbors[key]) {
			t.neigbors[key] = append(t.neigbors[key], p.Neighbor)
		}
		sort.Strings(t.neigbors[key])
		neigbors = fmt.Sprintf("%s", strings.Join(t.neigbors[key], ","))

		if v, ok := t.rib.Load(key); ok {

			if _, c := t.counters[key]; c {
				t.counters[key] = t.counters[key] + 1
			} else {
				t.counters[key] = 1
			}

			g.Log.Info(fmt.Sprintf("%s %s %s: SYNC COUNT:'%d' n:'%s' nn:['%s'] => %d",
				utils.GetGPid(), id, p.AsShortString(),
				t.counters[key], p.Neighbor, neigbors,
				len(t.neigbors[key])))

			q := v.(Path)
			if p.Equal(&q) {
				return err
			}
		}

		t.counters[key] = 1

		t.rib.Store(key, *p)
		g.Log.Info(fmt.Sprintf("%s %s %s: key:'%s' ACCEPTED -> ADDED n:'%s' nn:['%s'] => %d",
			utils.GetGPid(), id, key, p.AsShortString(),
			p.Neighbor, neigbors,
			len(t.neigbors[key])))
	}

	if p.Event == BGPEVENT_WITHDRAW {
		if _, ok := t.rib.Load(key); ok {
			if _, c := t.counters[key]; c {
				t.counters[key] = t.counters[key] - 1
			}

			index := -1
			if _, ok := t.neigbors[key]; ok {
				for i, n := range t.neigbors[key] {
					if n == p.Neighbor {
						index = i
					}
				}
				if index >= 0 {
					t.neigbors[key] = RemoveIndex(t.neigbors[key], index)
				}
			}

			sort.Strings(t.neigbors[key])
			neigbors := fmt.Sprintf("%s", strings.Join(t.neigbors[key], ","))

			mode := "WAIT"
			if t.counters[key] <= 0 || len(t.neigbors[key]) == 0 {
				mode = "DELETE"
				t.rib.Delete(key)
			}
			g.Log.Info(fmt.Sprintf("%s %s %s: key:'%s' WITHDRAW COUNT:'%d' -> '%d' '%s' n:'%s' nn:['%s']",
				utils.GetGPid(), id, key, p.AsShortString(),
				t.counters[key], t.counters[key]-1,
				mode, p.Neighbor, neigbors))
		}
	}
	return err
}

func (t *TMapPath) Get() []Path {
	var out []Path
	t.rib.Range(func(k, v interface{}) bool {
		out = append(out, v.(Path))
		return true
	})
	return out
}

type StatusOptionsOverrides struct {
	Json bool `json:"json"`
	Info bool `json:"info"`
}

func (b *StatusOptionsOverrides) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("json:'%t'", b.Json))
	return fmt.Sprintf("%s", strings.Join(out, ", "))
}

func (c *Core) genericClientCall(uri string) (Response, error) {
	var content []byte
	var code int
	var response Response
	var err error

	id := "(call)"

	if content, err, code = utils.HttpRequest(http.MethodGet,
		fmt.Sprintf("http://unix/%s", uri), "", nil,
		c.g.TransportUnix, "m3"); err != nil {

		c.g.Log.Error(fmt.Sprintf("%s %s error http call, err:'%s'",
			utils.GetGPid(), id, err))

		c.g.LogDump(id, string(content))

		// we could parse error message and retrieve a real list of error as a list "errors"
		// {"success":false,"errors":[{"code":502,"message":"bgp session is not established"}]}

		if err = json.Unmarshal(content, &response); err != nil {
			c.g.Log.Error(fmt.Sprintf("%s %s error parsing request, err:'%s'",
				utils.GetGPid(), id, err))
			return response, err
		}

		var out []string
		for _, e := range response.Errors {
			out = append(out, fmt.Sprintf("%d, %s", e.Code, e.Message))
		}
		return response, errors.New(fmt.Sprintf("%s", strings.Join(out, ",")))
	}

	c.g.Log.Debug(fmt.Sprintf("%s %s content recevied, len:'%d' code:'%d'",
		utils.GetGPid(), id, len(content), code))

	c.g.LogDump(id, string(content))

	if err = json.Unmarshal(content, &response); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error parsing request, err:'%s'",
			utils.GetGPid(), id, err))
		return response, err
	}

	return response, err
}

func (c *Core) networkStatus(StatusOptionsOverrides *StatusOptionsOverrides) (TObjects, error) {
	var err error

	uri := fmt.Sprintf("api/v3.1/network/objects")
	id := "(network)"

	var response Response
	var objects TObjects
	if response, err = c.genericClientCall(uri); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error http call, err:'%s'",
			utils.GetGPid(), id, err))
		return objects, err
	}

	for _, m := range response.Messages {
		c.g.Log.Debug(fmt.Sprintf("%s %s message:'%s'",
			utils.GetGPid(), id, m.Message))

		if err = json.Unmarshal([]byte(m.Message), &objects); err != nil {
			c.g.Log.Error(fmt.Sprintf("%s %s error parsing request, err:'%s'",
				utils.GetGPid(), id, err))
			return objects, err
		}

		c.g.Log.Debug(fmt.Sprintf("%s %s got objects %s",
			utils.GetGPid(), id, objects.AsString()))

		if StatusOptionsOverrides.Info {
			objects.DumpObjects(c.g)
		}
	}

	return objects, err
}

func (c *Core) bgpStatus(StatusOptionsOverrides *StatusOptionsOverrides) ([]Path, error) {
	var err error
	var paths []Path
	id := "(core)"

	if StatusOptionsOverrides != nil {
		c.g.Log.Debug(fmt.Sprintf("%s %s status overrides %s",
			utils.GetGPid(), id, StatusOptionsOverrides.AsString()))
	}

	uri := fmt.Sprintf("api/v3.1/bgp/paths")

	var response Response
	if response, err = c.genericClientCall(uri); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error http call, err:'%s'",
			utils.GetGPid(), id, err))
		return paths, err
	}

	for _, m := range response.Messages {
		c.g.Log.Debug(fmt.Sprintf("%s %s message:'%s'",
			utils.GetGPid(), id, m.Message))

		if err = json.Unmarshal([]byte(m.Message), &paths); err != nil {
			c.g.Log.Error(fmt.Sprintf("%s %s error parsing request, err:'%s'",
				utils.GetGPid(), id, err))
			return paths, err
		}

		c.g.Log.Debug(fmt.Sprintf("%s %s got paths:'%d'",
			utils.GetGPid(), id, len(paths)))

		for i, p := range paths {
			if StatusOptionsOverrides.Info {
				c.g.Log.Info(fmt.Sprintf("%s %s [%d]/[%d] path:'%s'",
					utils.GetGPid(), id, i, len(paths), p.AsString()))
			}
		}
	}

	return paths, err
}

type SyncOptionsOverrides struct {
	Dryrun   bool `json:"dry-run"`
	BgpStart bool `json:"bgp-start"`
	Timeout  int  `json:"timeout"`
}

func (b *SyncOptionsOverrides) AsString() string {
	var out []string
	out = append(out, fmt.Sprintf("dry-run:'%t'", b.Dryrun))
	out = append(out, fmt.Sprintf("bgp-start:'%t'", b.BgpStart))
	return fmt.Sprintf("%s", strings.Join(out, ", "))
}

func (c *Core) networkFlush(SyncOptionsOverrides *SyncOptionsOverrides) error {
	var err error
	id := "(sync)"

	c.g.Log.Debug(fmt.Sprintf("%s %s request to flush network objects",
		utils.GetGPid(), id))

	if SyncOptionsOverrides != nil {
		c.g.Log.Debug(fmt.Sprintf("%s %s sync (network) overrides %s",
			utils.GetGPid(), id, SyncOptionsOverrides.AsString()))
	}

	// Detecting all containers of specified types
	// in case of containers mode
	var ObjectsSources map[string][]*TObjectsSource
	if ObjectsSources, err = c.networkGetSources(); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error getting object sources, err:'%s'",
			utils.GetGPid(), id, err))
		return err
	}

	for t, ObjectsSources := range ObjectsSources {

		for _, ObjectsSource := range ObjectsSources {
			var objects TObjects

			if objects, ObjectsSource, err = c.networkGetObjects(ObjectsSource); err != nil {
				c.g.Log.Error(fmt.Sprintf("%s %s error getting network objects, err:'%s'",
					utils.GetGPid(), id, err))
				return err
			}

			c.g.Log.Debug(fmt.Sprintf("%s %s type:'%s' objects %s",
				utils.GetGPid(), id, t, objects.AsString()))

			// First we flush routes, as links removal also produces
			// routes to be deleted
			for i, r := range objects.Routes {

				oid := "[REMOVE]"
				c.g.Log.Debug(fmt.Sprintf("%s %s [%d]/[%d] %s route:%s",
					utils.GetGPid(), id, i, len(objects.Routes),
					oid, r.AsString()))

				if SyncOptionsOverrides.Dryrun {
					c.g.Log.Error(fmt.Sprintf("%s %s error, skipping route:'%s' removing, as dry-run is 'ON'",
						utils.GetGPid(), id, r.AsString()))
					continue
				}

				if err = r.Remove(c.g, ObjectsSource); err != nil {
					c.g.Log.Error(fmt.Sprintf("%s %s error removing route:'%s', err:'%s'",
						utils.GetGPid(), id, r.AsString(), err))
					continue
				}
			}

			// First we flush routes, as links removal also produces
			// routes to be deleted
			for i, r := range objects.Rules {

				oid := "[REMOVE]"
				c.g.Log.Debug(fmt.Sprintf("%s %s [%d]/[%d] %s rule:%s",
					utils.GetGPid(), id, i, len(objects.Rules),
					oid, r.AsString()))

				if SyncOptionsOverrides.Dryrun {
					c.g.Log.Error(fmt.Sprintf("%s %s error, skipping rule:'%s' removing, as dry-run is 'ON'",
						utils.GetGPid(), id, r.AsString()))
					continue
				}

				if err = r.Remove(c.g, ObjectsSource); err != nil {
					c.g.Log.Error(fmt.Sprintf("%s %s error removing rule:'%s', err:'%s'",
						utils.GetGPid(), id, r.AsString(), err))
					continue
				}
			}

			// The last: deleting networks objects: links, routes and rules
			for i, l := range objects.Links {
				oid := "[REMOVE]"
				c.g.Log.Debug(fmt.Sprintf("%s %s [%d]/[%d] %s link:%s",
					utils.GetGPid(), id, i, len(objects.Links),
					oid, l.AsString()))

				if SyncOptionsOverrides.Dryrun {
					c.g.Log.Error(fmt.Sprintf("%s %s error, skipping link:'%s' removing, as dry-run is 'ON'",
						utils.GetGPid(), id, l.AsString()))
					continue
				}

				if err = l.Remove(c.g, ObjectsSource); err != nil {
					c.g.Log.Error(fmt.Sprintf("%s %s error removing link:'%s', err:'%s'",
						utils.GetGPid(), id, l.AsString(), err))

					// Trying to flush all objects
					continue
				}
			}
		}
	}

	return err
}

type THttpPeers struct {
	IfPort string   `json:"if_port"`
	IfName string   `json:"if_name"`
	Lo4    string   `json:"lo4"`
	Lo6    string   `json:"lo6"`
	Name   string   `json:"name"`
	Ip     string   `json:"ip'`
	Id     int      `json:"id"`
	Tags   []string `json:"tags"`

	// Label offset to create statis mpls label
	// as label_offset + linkid, see TRAFFIC-12648
	LabelOffset int `json:"label_offset"`
}

func (t *THttpPeers) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("id:'%d'", t.Id))
	out = append(out, fmt.Sprintf("if_name:'%s'", t.IfName))
	out = append(out, fmt.Sprintf("if_port:'%s'", t.IfPort))
	out = append(out, fmt.Sprintf("-- ip:'%s'", t.Ip))
	out = append(out, fmt.Sprintf("lo4:'%s'", t.Lo4))
	out = append(out, fmt.Sprintf("lo6:'%s'", t.Lo6))
	out = append(out, fmt.Sprintf("name:'%s'", t.Name))
	out = append(out, fmt.Sprintf("label_offset:'%d'", t.LabelOffset))
	out = append(out, fmt.Sprintf("-- tags:['%s']", strings.Join(t.Tags, ",")))

	return strings.Join(out, ",")
}

const (
	NEIGHBOUR_DEFAULT = "::1"
	ASN_DEFAULT       = 13238
	LOWER_DEFAULT     = 10000
)

func (c *Core) httpGetPaths() ([]Path, error) {
	var paths []Path
	var err error

	id := "(http) (paths)"
	url := c.g.Opts.M3.Source

	c.g.Log.Debug(fmt.Sprintf("%s %s url:'%s'",
		utils.GetGPid(), id, url))

	if len(url) == 0 {
		err = errors.New("endpoint url for source not defined")
		return paths, err
	}

	agent := fmt.Sprintf("%s/%s", config.PROGRAM_NAME,
		c.g.Opts.Runtime.Version)

	c.g.Log.Debug(fmt.Sprintf("%s %s agent:'%s'",
		utils.GetGPid(), id, agent))

	var content []byte
	var code int
	// Here we use skip tls as d2 https uses self-signed
	// certificate (exporting some api methods)

	transport := c.g.TransportSkipTls

	t0 := time.Now()
	if content, err, code = utils.HttpRequest(http.MethodGet, url, "", []byte{},
		transport, agent); err != nil {

		c.g.Log.Error(fmt.Sprintf("%s error http call, url:'%s', code:'%d', err:'%s'",
			utils.GetGPid(), url, code, err))
		return paths, err
	}

	c.g.Log.Debug(fmt.Sprintf("%s %s remote call, code:'%d', length:'%d' answered in '%s' OK",
		utils.GetGPid(), id, code, len(content),
		time.Since(t0)))

	var Peers []THttpPeers
	if err = json.Unmarshal([]byte(content), &Peers); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s error unmarshalling, remote answer, url:'%s' err:'%s'",
			utils.GetGPid(), url, err))
		return paths, err
	}

	c.g.Log.Debug(fmt.Sprintf("%s %s peers count:'%d'",
		utils.GetGPid(), id, len(Peers)))

	links := c.g.Opts.Objects.Links

	max := 1000
	for i, p := range Peers {
		// displaying only first max
		if i < max {
			c.g.Log.Debug(fmt.Sprintf("%s %s peer [%d]/[%d]:[%d] %s",
				utils.GetGPid(), id, i, max, len(Peers),
				p.AsString()))
		}

		// Checking if ip is an ip4/ip6
		var path Path

		path.Safi = api.Family_SAFI_MPLS_VPN
		path.Neighbor = NEIGHBOUR_DEFAULT
		path.Identifier = 0
		path.LocalIdentifier = 0
		path.Event = BGPEVENT_ACCEPTED
		path.SourceAsn = ASN_DEFAULT

		ip := net.ParseIP(p.Ip)
		if ip == nil {
			c.g.Log.Error(fmt.Sprintf("%s %s error parsing ip:'%s' from peer:'%s', skipping",
				utils.GetGPid(), id, p.Ip, p.AsString()))
			continue
		}

		path.PrefixLen = 0

		path.Afi = api.Family_AFI_IP
		path.Prefix = "0.0.0.0"

		if strings.Contains(p.Ip, ":") {
			path.Afi = api.Family_AFI_IP6
			path.Prefix = "::"
		}

		label := p.LabelOffset + p.Id
		path.Labels = append(path.Labels, uint32(label))

		var cm Community
		cm.High = ASN_DEFAULT
		cm.Low = uint32(LOWER_DEFAULT) + uint32(p.Id)

		var rcm ResolvedCommunity
		rcm.Linkid = uint32(p.Id)
		rcm.Community = append(rcm.Community, cm)

		path.Community = rcm

		path.NextHops = append(path.NextHops, p.Lo6)

		if int(path.Community.Linkid) < links.Match.Min ||
			int(path.Community.Linkid) > links.Match.Max {

			c.g.Log.Debug(fmt.Sprintf("%s %s path:'%s' skipping linkid:'%d' as not matched to be in interval of '[%d %d]'",
				utils.GetGPid(), id, path.AsString(), path.Community.Linkid, links.Match.Min,
				links.Match.Max))

			continue

		}

		c.g.Log.Debug(fmt.Sprintf("%s %s path [%d]/[%d] %s",
			utils.GetGPid(), id, i, len(Peers),
			path.AsString()))

		paths = append(paths, path)
	}

	return paths, err
}

func (c *Core) sourceValidatePaths(paths []Path) error {
	var err error

	// number of paths should be a positive number
	if len(paths) == 0 {
		err = errors.New("number of paths recevied should be a positive number")
		return err
	}

	return err
}

func (c *Core) sourceGetPaths() ([]Path, error) {
	var paths []Path
	var err error

	id := "(source) (paths)"

	c.g.Log.Debug(fmt.Sprintf("%s %s sync (network) source:'%s'",
		utils.GetGPid(), id, config.SourceAsString(c.g.Opts.Runtime.Source)))

	switch c.g.Opts.Runtime.Source {
	case config.SOURCE_BGP:
		if paths, err = c.bgpGetPaths(); err != nil {
			c.g.Log.Error(fmt.Sprintf("%s %s error getting paths, err:'%s'",
				utils.GetGPid(), id, err))
			return paths, err
		}
	case config.SOURCE_HTTP:
		if paths, err = c.httpGetPaths(); err != nil {
			c.g.Log.Error(fmt.Sprintf("%s %s error getting paths, err:'%s'",
				utils.GetGPid(), id, err))
			return paths, err
		}
	}

	c.g.Log.Debug(fmt.Sprintf("%s %s got paths:'%d'",
		utils.GetGPid(), id, len(paths)))

	if err = c.sourceValidatePaths(paths); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error validating paths, err:'%s'",
			utils.GetGPid(), id, err))
		return paths, err
	}

	return paths, err
}

// Main function to sync bgp objects and network (on host system)
func (c *Core) networkSync(SyncOptionsOverrides *SyncOptionsOverrides) error {
	var err error
	id := "(sync)"

	c.g.Log.Debug(fmt.Sprintf("%s %s request to sync network objects",
		utils.GetGPid(), id))

	if SyncOptionsOverrides != nil {
		c.g.Log.Debug(fmt.Sprintf("%s %s sync (network) overrides %s",
			utils.GetGPid(), id, SyncOptionsOverrides.AsString()))
	}

	class := c.g.Opts.Runtime.NetworkClass

	// Dumping some parts of configuration for debug
	if class == config.NETWORK_CLASS_TUNNELS {
		bypass := c.g.Opts.Network.Tunnels.Bypass
		if bypass != nil {
			c.g.Log.Debug(fmt.Sprintf("%s %s bypass configured:'%s'",
				utils.GetGPid(), id, bypass.AsString()))
		}
	}

	var paths []Path
	if paths, err = c.sourceGetPaths(); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error getting object sources, err:'%s'",
			utils.GetGPid(), id, err))
		return err
	}

	// Detecting all containers of specified types
	// in case of containers mode
	var ObjectsSources map[string][]*TObjectsSource
	if ObjectsSources, err = c.networkGetSources(); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error getting object sources, err:'%s'",
			utils.GetGPid(), id, err))
		return err
	}

	// TODO: validating an array of paths: (1) mpls label should be exactly 1?
	// (2) nexthops should be exactly 1? (3) from different neighbors a triple
	// prefix+link should point to the same mpls array?

	for i, p := range paths {
		c.g.Log.Info(fmt.Sprintf("%s %s [%d]/[%d] path:'%s'",
			utils.GetGPid(), id, i, len(paths), p.AsShortString()))
	}

	for t, ObjectsSources := range ObjectsSources {

		for i, ObjectsSource := range ObjectsSources {
			var dst TObjects

			c.g.Log.Debug(fmt.Sprintf("%s %s getting network objects [%d]/[%d] type:'%s' container:'%s'",
				utils.GetGPid(), id, i, len(ObjectsSources), t, ObjectsSource.container))

			if dst, ObjectsSource, err = c.networkGetObjects(ObjectsSource); err != nil {
				c.g.Log.Error(fmt.Sprintf("%s %s error getting network objects, err:'%s'",
					utils.GetGPid(), id, err))
				return err
			}

			c.g.Log.Debug(fmt.Sprintf("%s %s dst objects %s", utils.GetGPid(),
				id, dst.AsString()))

			// Generating from paths corresponding triple:
			// links, routes and rules
			var Ip *Ip
			if Ip, err = CreateIp(c.g); err != nil {
				c.g.Log.Error(fmt.Sprintf("%s %s error creating ip, err:'%s'",
					utils.GetGPid(), id, err))
				return err
			}

			var src TObjects
			if src, err = Ip.generateObjects(ObjectsSource, class, paths); err != nil {
				c.g.Log.Error(fmt.Sprintf("%s %s error syncing paths, err:'%s'",
					utils.GetGPid(), id, err))
				return err
			}

			c.g.Log.Debug(fmt.Sprintf("%s %s src (generated) objects %s", utils.GetGPid(),
				id, dst.AsString()))

			if err = c.networkActions(src, dst, ObjectsSource, SyncOptionsOverrides); err != nil {
				c.g.Log.Error(fmt.Sprintf("%s %s error generating actions, err:'%s'",
					utils.GetGPid(), id, err))
				return err
			}
		}

	}
	return err
}

// resolving container fqdn to first AAAA
func (c *Core) networkResolveContainer(fqdn string) (string, error) {
	var err error
	var addr string

	id := "(resolve)"

	var answer Answer
	if answer, err = Resolve("", fqdn, dns.TypeAAAA); err != nil {
		return addr, err
	}
	c.g.Log.Debug(fmt.Sprintf("%s %s  fqdn:'%s' resolved as %s", utils.GetGPid(),
		id, fqdn, answer.AsString()))

	if len(answer.Records) == 0 {
		err = errors.New(fmt.Sprintf("expected AAAA record for fqdn:'%s' is not resolved", fqdn))
		return addr, err
	}

	// HEADSUP: uncertanty if AAAA records more than 1
	addr = answer.Records[0]

	return addr, err
}

// getting object sources w.r.t mode we use
func (c *Core) networkGetSources() (map[string][]*TObjectsSource, error) {

	id := "(network) (sources)"

	var err error
	ObjectsSources := make(map[string][]*TObjectsSource)

	mode := c.g.Opts.Runtime.NetworkMode

	if mode == config.NETWORK_MODE_HOST {
		ObjectsSource := new(TObjectsSource)
		ObjectsSources["host"] = append(ObjectsSources["host"], ObjectsSource)
	}

	if mode == config.NETWORK_MODE_CONTAINER {

		var client *lxd.Client
		if client, err = lxd.CreateClient(c.g); err != nil {
			c.g.Log.Error(fmt.Sprintf("%s %s error connecting to lxd, err:'%s'",
				utils.GetGPid(), id, err))
			return ObjectsSources, err
		}

		prefixes := c.g.Opts.Network.Container.Types

		var containers []lxd.Container
		if containers, err = client.ListContainers(prefixes); err != nil {
			c.g.Log.Error(fmt.Sprintf("%s %s error listing containers, err:'%s'",
				utils.GetGPid(), id, err))
			return ObjectsSources, err
		}

		c.g.Log.Debug(fmt.Sprintf("%s %s (containers) received containers:'%d'",
			utils.GetGPid(), id, len(containers)))

		// No containers detected
		if len(containers) == 0 {
			err = errors.New("no containers detected")
			c.g.Log.Error(fmt.Sprintf("%s %s error detecting containers, err:'%s'",
				utils.GetGPid(), id, err))
			return ObjectsSources, err
		}

		for i, C := range containers {

			c.g.Log.Info(fmt.Sprintf("%s %s [%d]/[%d] %s",
				utils.GetGPid(), id, i, len(containers), C.AsString()))

			// We need only containers in running state
			if C.Status == "Running" {
				ObjectsSource := new(TObjectsSource)
				ObjectsSource.client = client
				ObjectsSource.container = C.Name
				ObjectsSources[C.Type] = append(ObjectsSources[C.Type], ObjectsSource)
			}
		}

		for t, cc := range ObjectsSources {
			var out []string
			for _, c := range cc {
				out = append(out, c.container)
			}
			c.g.Log.Debug(fmt.Sprintf("%s %s (containers) sync enabled of type:'%s' -> [%d]:['%s']",
				utils.GetGPid(), id, t, len(out), strings.Join(out, ",")))
		}
	}

	return ObjectsSources, err
}

// we need to detect current network objects: ip link, ip route table,
// ip rule at container or current host
func (c *Core) networkGetObjects(ObjectsSource *TObjectsSource) (TObjects, *TObjectsSource, error) {
	var err error
	var objects TObjects

	id := "(network) (get) (objects)"

	mode := c.g.Opts.Runtime.NetworkMode
	class := c.g.Opts.Runtime.NetworkClass

	c.g.Log.Debug(fmt.Sprintf("%s %s request to get network objects, mode:'%s' class:'%s'",
		utils.GetGPid(), id, config.NetworkModeAsString(mode),
		config.NetworkClassAsString(class)))

	var Ip *Ip
	if Ip, err = CreateIp(c.g); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error creating ip, err:'%s'",
			utils.GetGPid(), id, err))
		return objects, ObjectsSource, err
	}

	// if we have auto for bypass routes we need to detect
	// a route ip4/ip6. Checking if bypass enabled
	bypass := c.g.Opts.Network.Tunnels.Bypass
	if class == config.NETWORK_CLASS_TRANSITS {
		bypass = c.g.Opts.Network.Transits.Bypass
	}

	if bypass != nil && bypass.Enabled {
		if bypass.Device == "auto" || bypass.GatewayIp6 == "auto" {
			Afi := ROUTE_IP6
			var route *NetworkRoute
			if route, err = Ip.getDefaultRoute(ObjectsSource, Afi); err != nil {
				c.g.Log.Error(fmt.Sprintf("%s %s error detecting default route, err:'%s'",
					utils.GetGPid(), id, err))
				return objects, ObjectsSource, err
			}

			switch class {
			case config.NETWORK_CLASS_TRANSITS:
				c.g.Opts.Network.Transits.Bypass.DeviceValue = route.Device
				c.g.Opts.Network.Transits.Bypass.GatewayIp6Value = route.Gateway
			case config.NETWORK_CLASS_TUNNELS:
				c.g.Opts.Network.Tunnels.Bypass.DeviceValue = route.Device
				c.g.Opts.Network.Tunnels.Bypass.GatewayIp6Value = route.Gateway
			}

			c.g.Log.Debug(fmt.Sprintf("%s %s [%s] fqdn:'%s' setting default bypass: device:'%s' gateway:'%s'",
				utils.GetGPid(), id, RouteAsString(Afi), ObjectsSource.container, route.Device,
				route.Gateway))

			Afi = ROUTE_IP4
			if route, err = Ip.getDefaultRoute(ObjectsSource, Afi); err != nil {
				c.g.Log.Error(fmt.Sprintf("%s %s error detecting default route, err:'%s'",
					utils.GetGPid(), id, err))
				return objects, ObjectsSource, err
			}

			switch class {
			case config.NETWORK_CLASS_TRANSITS:
				c.g.Opts.Network.Transits.Bypass.GatewayIp4Value = route.Gateway
			case config.NETWORK_CLASS_TUNNELS:
				c.g.Opts.Network.Tunnels.Bypass.GatewayIp4Value = route.Gateway
			}

			c.g.Log.Debug(fmt.Sprintf("%s %s [%s] fqdn:'%s' setting default bypass: device:'%s' gateway:'%s'",
				utils.GetGPid(), id, RouteAsString(Afi), ObjectsSource.container, route.Device,
				route.Gateway))
		}

		switch class {
		case config.NETWORK_CLASS_TRANSITS:
			if bypass.Device != "auto" {
				c.g.Opts.Network.Transits.Bypass.DeviceValue = bypass.Device
			}

			if bypass.GatewayIp6 != "auto" {
				c.g.Opts.Network.Transits.Bypass.GatewayIp6Value = bypass.GatewayIp6
			}

			if bypass.GatewayIp4 != "auto" {
				c.g.Opts.Network.Transits.Bypass.GatewayIp4Value = bypass.GatewayIp4
			}

		case config.NETWORK_CLASS_TUNNELS:
			if bypass.Device != "auto" {
				c.g.Opts.Network.Tunnels.Bypass.DeviceValue = bypass.Device
			}

			if bypass.GatewayIp6 != "auto" {
				c.g.Opts.Network.Tunnels.Bypass.GatewayIp6Value = bypass.GatewayIp6
			}

			if bypass.GatewayIp4 != "auto" {
				c.g.Opts.Network.Tunnels.Bypass.GatewayIp4Value = bypass.GatewayIp4
			}
		}
	}

	// If we have tunnels routes (borders loopback)
	// enabled, we need to detect device and gateway-ip6
	loopbacks := c.g.Opts.Network.Tunnels.TunnelsRoutes
	if loopbacks.Enabled && (loopbacks.Device == "auto" ||
		loopbacks.GatewayIp6 == "auto") {
		Afi := ROUTE_IP6
		var route *NetworkRoute
		if route, err = Ip.getDefaultRoute(ObjectsSource, Afi); err != nil {
			c.g.Log.Error(fmt.Sprintf("%s %s error detecting default route, err:'%s'",
				utils.GetGPid(), id, err))
			return objects, ObjectsSource, err
		}

		if loopbacks.Device == "auto" {
			c.g.Opts.Network.Tunnels.TunnelsRoutes.DeviceValue = route.Device
		}

		if loopbacks.GatewayIp6 == "auto" {
			c.g.Opts.Network.Tunnels.TunnelsRoutes.GatewayIp6Value = route.Gateway
		}

		c.g.Log.Debug(fmt.Sprintf("%s %s [%s] fqdn:'%s' loopbacks routes settings: device:'%s' gateway:'%s'",
			utils.GetGPid(), id, RouteAsString(Afi), ObjectsSource.container, route.Device,
			route.Gateway))
	}

	// Fallback to real values from configuration
	if loopbacks.Enabled && loopbacks.Device != "auto" {
		c.g.Opts.Network.Tunnels.TunnelsRoutes.DeviceValue =
			loopbacks.Device
	}

	if loopbacks.Enabled && loopbacks.GatewayIp6 != "auto" {
		c.g.Opts.Network.Tunnels.TunnelsRoutes.GatewayIp6Value =
			loopbacks.GatewayIp6
	}

	// Only for tunnels we need links managment
	if class == config.NETWORK_CLASS_TUNNELS {

		// Detecting IP6 address via dns lookup
		// if tunnels localaddress set to "auto"
		local := c.g.Opts.Network.Tunnels.LocalAddress
		fqdn := c.g.Opts.Runtime.Hostname

		if mode == config.NETWORK_MODE_CONTAINER {
			fqdn = ObjectsSource.container
		}

		c.g.Opts.Network.Tunnels.LocalAddressResolved =
			c.g.Opts.Network.Tunnels.LocalAddress

		if local == "auto" {
			var addr string
			if addr, err = c.networkResolveContainer(fqdn); err != nil {
				c.g.Log.Error(fmt.Sprintf("%s %s error detecting tunnel ip host:'%s', err:'%s'",
					utils.GetGPid(), id, fqdn, err))
				return objects, nil, err
			}

			c.g.Opts.Network.Tunnels.LocalAddressResolved = addr
		}

		c.g.Log.Debug(fmt.Sprintf("%s %s fqdn:'%s' tunnel local-address config '%s' -> '%s'",
			utils.GetGPid(), id, ObjectsSource.container, local,
			c.g.Opts.Network.Tunnels.LocalAddressResolved))

		// Getting tunnel6 interfaces with prefix configured
		prefix := c.g.Opts.Network.Tunnels.DevicePrefix
		linktype := LINK_TYPE_TUNNEL6

		if len(prefix) == 0 {
			err = errors.New("device prefix is empty")
			c.g.Log.Error(fmt.Sprintf("%s %s error getting links objects, err:'%s'",
				utils.GetGPid(), id, err))
			return objects, ObjectsSource, err
		}

		var links []NetworkLink
		if links, err = Ip.getSpecficNetworkLinks(ObjectsSource,
			linktype, prefix); err != nil {

			c.g.Log.Error(fmt.Sprintf("%s %s error getting networks links, err:'%s'",
				utils.GetGPid(), id, err))
			return objects, ObjectsSource, err
		}

		c.g.Log.Debug(fmt.Sprintf("%s %s fqdn:'%s' links type:'%s' prefix:'%s' -> count:'%d'",
			utils.GetGPid(), id, ObjectsSource.container, linktype, prefix, len(links)))

		for i, l := range links {

			// TODO: Filtering here
			objects.Links = append(objects.Links, l)

			c.g.Log.Info(fmt.Sprintf("%s %s fqdn:'%s' links [%d]/[%d] '%s' %s",
				utils.GetGPid(), id, ObjectsSource.container, i,
				len(links), linktype,
				l.AsString()))
		}
	}

	// Getting tabled routes proto could be "all" (for tunnels scheme)
	// and "xorp" for transit schemes
	proto := c.g.Opts.Network.Tunnels.Proto
	if class == config.NETWORK_CLASS_TRANSITS {
		proto = c.g.Opts.Network.Transits.Proto
	}

	var routes []NetworkRoute
	if routes, err = Ip.getSpecificNetworkRoutes(ObjectsSource, proto); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error getting routes, err:'%s'",
			utils.GetGPid(), id, err))
		return objects, ObjectsSource, err
	}
	c.g.Log.Debug(fmt.Sprintf("%s %s fqdn:'%s' routes proto:'%s' -> count:'%d'",
		utils.GetGPid(), id, ObjectsSource.container,
		proto, len(routes)))

	for i, r := range routes {
		c.g.Log.Info(fmt.Sprintf("%s %s routes [%d]/[%d] %s",
			utils.GetGPid(), id, i, len(routes),
			r.AsString()))

		objects.Routes = append(objects.Routes, r)
	}

	// Getting tabled rules
	var rules []NetworkRule
	if rules, err = Ip.getFwmarkNetworkRules(ObjectsSource); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error getting rules, err:'%s'",
			utils.GetGPid(), id, err))
		return objects, ObjectsSource, err
	}
	c.g.Log.Debug(fmt.Sprintf("%s %s fqdn:'%s' rules -> count:'%d'",
		utils.GetGPid(), id, ObjectsSource.container,
		len(rules)))

	for i, r := range rules {
		c.g.Log.Info(fmt.Sprintf("%s %s fqdn:'%s' rules [%d]/[%d] %s",
			utils.GetGPid(), id, ObjectsSource.container, i, len(rules),
			r.AsString()))

		objects.Rules = append(objects.Rules, r)
	}

	return objects, ObjectsSource, err
}

// Generating a difference between src and dst objects
// also constructing actions to be applied later
func (c *Core) networkActions(src TObjects, dst TObjects,
	ObjectsSource *TObjectsSource,
	SyncOptionsOverrides *SyncOptionsOverrides) error {
	var err error

	id := fmt.Sprintf("(actions) '%s'", ObjectsSource.container)

	c.g.Log.Debug(fmt.Sprintf("%s %s request to generate actions",
		utils.GetGPid(), id))

	c.g.Log.Debug(fmt.Sprintf("%s %s [SRC] '%s'",
		utils.GetGPid(), id, src.AsString()))

	c.g.Log.Debug(fmt.Sprintf("%s %s [DST] '%s'",
		utils.GetGPid(), id, dst.AsString()))

	var Ip *Ip
	if Ip, err = CreateIp(c.g); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error creating ip, err:'%s'",
			utils.GetGPid(), id, err))
		return err
	}

	count := 0
	totals := 0

	// Converting rules, routes and links and getting actions

	// TODO: in two modes, first in ACTIONSMODE_CALC and then in
	// ACTIONSMODE_APPLY to set the right order from possible
	// two variants: if a link should be created

	// We have an issue with right order of operations: if we need
	// to delete objects - first deleting routes + rules and then
	// links. If we need create - fist create link and then all the rest.

	// However, all issues could be fixed later on the next
	// syncronizaion (if any)

	lsrc := Ip.GetObjectLinksSlice(src.Links)
	ldst := Ip.GetObjectLinksSlice(dst.Links)
	if count, err = c.networkActionsApply(ACTIONSMODE_APPLY, "link", lsrc,
		ldst, ObjectsSource, SyncOptionsOverrides); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error applying actions, err:'%s'",
			utils.GetGPid(), id, err))
		return err
	}
	totals += count

	tsrc := Ip.GetObjectRoutesSlice(src.Routes)
	tdst := Ip.GetObjectRoutesSlice(dst.Routes)
	if count, err = c.networkActionsApply(ACTIONSMODE_APPLY, "route", tsrc,
		tdst, ObjectsSource, SyncOptionsOverrides); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error applying actions, err:'%s'",
			utils.GetGPid(), id, err))
		return err
	}
	totals += count

	rsrc := Ip.GetObjectRuleSlice(src.Rules)
	rdst := Ip.GetObjectRuleSlice(dst.Rules)
	if count, err = c.networkActionsApply(ACTIONSMODE_APPLY, "rule", rsrc,
		rdst, ObjectsSource, SyncOptionsOverrides); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error applying actions, err:'%s'",
			utils.GetGPid(), id, err))
		return err
	}
	totals += count

	c.g.Log.Debug(fmt.Sprintf("%s %s syncronization actions count:'%d'",
		utils.GetGPid(), id, totals))
	if totals == 0 {
		c.g.Log.Info(fmt.Sprintf("%s %s objects links:[%d]/[%d] route:[%d]/[%d] rules:[%d]/[%d] syncronized: 'OK'",
			utils.GetGPid(), id, len(lsrc), len(ldst), len(tsrc), len(tdst),
			len(rsrc), len(rdst)))
	}

	return err
}

const (
	ACTIONSMODE_APPLY = 1001
	ACTIONSMODE_CALC  = 1002
)

// action indicates if generated actions should be applied
// or just calculated, see network actions for details
func (c *Core) networkActionsApply(action int, class string, src []Object, dst []Object,
	ObjectsSource *TObjectsSource, SyncOptionsOverrides *SyncOptionsOverrides) (int, error) {
	var err error
	var actions int

	id := "(actions)"

	var Ip *Ip
	if Ip, err = CreateIp(c.g); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error creating ip, err:'%s'",
			utils.GetGPid(), id, err))
		return actions, err
	}

	var Actions Actions
	if Actions, err = Ip.detectObjectActions(src,
		dst, ObjectsSource, class); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error detecting actions, err:'%s'",
			utils.GetGPid(), id, err))
		return actions, err
	}

	actions += len(Actions.CreateActions)
	actions += len(Actions.RemoveActions)

	if (len(Actions.CreateActions) > 0 || len(Actions.RemoveActions) > 0) &&
		SyncOptionsOverrides.Dryrun {
		c.g.Log.Error(fmt.Sprintf("%s %s actions generated but dry-run is 'ON', skipping actions",
			utils.GetGPid(), id))
	}

	// metrics are generated and sent to solomon
	var metrics []monitor.Value
	var w monitor.Value

	w.Value = float64(len(Actions.CreateActions))
	w.Id = fmt.Sprintf("actions-%s-create", class)
	metrics = append(metrics, w)

	w.Value = float64(len(Actions.RemoveActions))
	w.Id = fmt.Sprintf("actions-%s-remove", class)
	metrics = append(metrics, w)

	for i, s := range metrics {
		c.g.Log.Debug(fmt.Sprintf("%s %s [%d]/[%d] metric:'%s'",
			utils.GetGPid(), id, i, len(metrics),
			s.AsString()))
	}

	if err = c.g.Monitor.SendSolomonsMetrics(metrics, true); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error send solomon metrics, err:'%s'",
			utils.GetGPid(), id, err))
	}

	if !SyncOptionsOverrides.Dryrun {
		if action == ACTIONSMODE_APPLY {
			if err = Actions.Apply(c.g); err != nil {
				c.g.Log.Error(fmt.Sprintf("%s %s error applying actions, err:'%s'",
					utils.GetGPid(), id, err))
				return actions, err
			}
		}
	}

	return actions, err
}

func (c *Core) serverLogrotate(LogrotateOptions *config.LogrotateOptions) error {
	var err error
	id := "(core)"

	uri := fmt.Sprintf("api/v3.1/server/logrotate")
	if LogrotateOptions != nil && LogrotateOptions.Move {
		uri = fmt.Sprintf("%s?operation=move", uri)
	}

	var response Response
	if response, err = c.genericClientCall(uri); err != nil {
		c.g.Log.Error(fmt.Sprintf("%s %s error http call, err:'%s'",
			utils.GetGPid(), id, err))
		return err
	}
	for _, m := range response.Messages {
		c.g.Log.Debug(fmt.Sprintf("%s %s message:'%s'",
			utils.GetGPid(), id, m.Message))
	}
	return err
}
