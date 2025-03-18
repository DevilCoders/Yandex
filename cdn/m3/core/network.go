package core

import (
	"bytes"
	"crypto/sha1"
	"encoding/binary"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"os/exec"
	"regexp"
	"sort"
	"strconv"
	"strings"

	"a.yandex-team.ru/cdn/m3/cmd/config"
	"a.yandex-team.ru/cdn/m3/lxd"
	"a.yandex-team.ru/cdn/m3/utils"
)

// source of network objects: could be
// a current host, a container under lxd
type TObjectsSource struct {

	// pointer to lxd client
	client *lxd.Client

	// container name
	container string `json:"container"`
}

// ip link object
type NetworkLink struct {
	Ifname string `json:"ifname"`

	// not sure about if is needed to
	// set as link tunnel create
	Mtu int `json:"mtu"`

	// Some useful flags
	Flags []string `json:"flags"`

	// Please be sure to normilize addresses
	// to net.Ip notation to compare
	Remote string `json:"broadcast"`
	Local  string `json:"address"`

	EncapDestPort uint32 `json:"encap-dport"`

	// qlen should be set to some more
	// than default 1000
	Txqlen int `json:"txqlen"`

	LinkType string `json:"link_type"`
}

const (
	LINKFLAG_UP   = "UP"
	LINKFLAG_DOWN = "DOWN"
)

func (n *NetworkLink) IfUp() bool {
	return utils.StringInSlice(LINKFLAG_UP, n.Flags)
}

func (n *NetworkLink) IfDown() bool {
	return utils.StringInSlice(LINKFLAG_DOWN, n.Flags)
}

// Generic execution on host system on in container, optional
// parmeters class and op - just for debugging
func GenericExec(g *config.CmdGlobal, ObjectsSource *TObjectsSource,
	args []string, class string, op string) ([]byte, error) {

	var err error
	var output []byte
	ip := "/sbin/ip"

	if ObjectsSource != nil && ObjectsSource.client != nil {

		/*
			if ObjectsSource.client == nil {
				err = errors.New("lxd connection not found")
				g.Log.Error(fmt.Sprintf("%s error executing command, err:'%s'",
					utils.GetGPid(), err))
				return output, err
			}
		*/

		command := []string{ip}
		command = append(command, args...)
		container := ObjectsSource.container

		var Response *lxd.Response
		if Response, err = ObjectsSource.client.Exec(container,
			command); err != nil {

			// Response could be nil if no any commands executed
			// or container is stopped
			if Response != nil {
				g.LogDump(utils.GetGPid(), string(Response.StdErr))
			}

			g.Log.Error(fmt.Sprintf("%s error executing command, err:'%s'",
				utils.GetGPid(), err))
			return output, err
		}

		g.LogDump(utils.GetGPid(), string(Response.StdOut))

		output = Response.StdOut
	} else {

		if output, err = exec.Command(ip,
			args...).CombinedOutput(); err != nil {

			g.LogDump(utils.GetGPid(), string(output))
			g.Log.Error(fmt.Sprintf("%s error executing command, err:'%s'",
				utils.GetGPid(), err))
			return output, err
		}
		g.LogDump(utils.GetGPid(), string(output))
	}

	g.LogDump(utils.GetGPid(), string(output))

	container := ""
	if ObjectsSource != nil {
		container = fmt.Sprintf(" c:'%s'", ObjectsSource.container)
	}

	g.Log.Debug(fmt.Sprintf("%s %s%s '%s' command:['%s %s']: 'OK'",
		utils.GetGPid(), op, container, class, ip,
		strings.Join(args, " ")))
	return output, err
}

const (
	O_CREATE = "CREATE"
	O_REMOVE = "REMOVE"
)

func (n NetworkLink) Create(g *config.CmdGlobal, ObjectsSource *TObjectsSource) error {
	var err error

	tunnels := g.Opts.Network.Tunnels

	// Creating link, setting mtu and set in to UP
	args := []string{"link", "add", "name", n.Ifname,
		"numtxqueues", fmt.Sprintf("%d", tunnels.NumTxQueues),
		"numrxqueues", fmt.Sprintf("%d", tunnels.NumRxQueues),
		"txqueuelen", fmt.Sprintf("%d", tunnels.TxQueueLen),
		"type", "ip6tnl", "remote", n.Remote, "local", n.Local,
		"encap", "fou", "encap-sport", "auto", "encap-dport",
		fmt.Sprintf("%d", n.EncapDestPort), "encaplimit", "none"}

	if _, err = GenericExec(g, ObjectsSource, args, "link", O_CREATE); err != nil {
		return err
	}

	args = []string{"link", "set", n.Ifname, "mtu",
		fmt.Sprintf("%d", n.Mtu)}
	if _, err = GenericExec(g, ObjectsSource, args, "link", O_CREATE); err != nil {
		return err
	}

	args = []string{"link", "set", n.Ifname, "up"}
	if _, err = GenericExec(g, ObjectsSource, args, "link", O_CREATE); err != nil {
		return err
	}

	return err
}

func (n NetworkLink) Remove(g *config.CmdGlobal, ObjectsSource *TObjectsSource) error {
	var err error

	if n.IfUp() {
		args := []string{"link", "set", n.Ifname, "down"}
		if _, err = GenericExec(g, ObjectsSource, args, "link", O_REMOVE); err != nil {
			return err
		}
	}

	args := []string{"link", "delete", n.Ifname}
	if _, err = GenericExec(g, ObjectsSource, args, "link", O_REMOVE); err != nil {
		return err
	}

	return err
}

func (n NetworkLink) AsString() string {
	return fmt.Sprintf("linktype:'%s' ifname:'%s' local:'%s' remote:'%s' mtu:'%d', qlen:'%d', flags:'[%s]'",
		n.LinkType, n.Ifname, n.Local, n.Remote, n.Mtu, n.Txqlen,
		strings.Join(n.Flags, ", "))
}

func (n NetworkLink) Key() string {
	return fmt.Sprintf("%s-%s-%s-%s-%d-%s",
		n.LinkType, n.Ifname, n.Local, n.Remote, n.Mtu, n.Txqlen,
		strings.Join(n.Flags, ", "))
}

// placeholder for ip command
type Ip struct {
	g *config.CmdGlobal
}

func CreateIp(g *config.CmdGlobal) (*Ip, error) {
	var err error

	var ip Ip
	ip.g = g

	return &ip, err
}

// Parsing ip link output and gathering links data
func (i *Ip) getNetworkLinks(ObjectsSource *TObjectsSource) ([]NetworkLink, error) {
	var err error
	var out []NetworkLink

	args := []string{"-j", "link", "show"}

	var output []byte

	if output, err = GenericExec(i.g, ObjectsSource, args, "show", "default"); err != nil {
		i.g.Log.Error(fmt.Sprintf("%s error executing command, err:'%s'",
			utils.GetGPid(), err))
		return out, err
	}

	/*
		if output, err = exec.Command("/sbin/ip", "-j", "link", "show").CombinedOutput(); err != nil {
			i.g.LogDump(utils.GetGPid(), string(output))
			i.g.Log.Error(fmt.Sprintf("%s error executing command, err:'%s'",
				utils.GetGPid(), err))
			return out, err
		}
	*/

	i.g.LogDump(utils.GetGPid(), string(output))
	if err = json.Unmarshal(output, &out); err != nil {
		i.g.Log.Error(fmt.Sprintf("%s error encoding executing command, err:'%s'",
			utils.GetGPid(), err))
		return out, err
	}

	return out, err
}

// Specific function to get a list of network links
// with prefix specified and of link type
const (
	LINK_TYPE_TUNNEL6 = "tunnel6"
	LINK_TYPE_ETHER   = "ether"
)

const (
	// Default txqlen for created
	// tunnels? should be actually set
	// to a speed of interface, 10G = 10000
	TUNNEL6_TXQLEN = 1000

	// Should be later configured via config
	TUNNEL6_MTU = 8952
)

func (i *Ip) getSpecficNetworkLinks(ObjectsSource *TObjectsSource,
	LinkType string, LinkPrefix string) ([]NetworkLink, error) {
	var err error
	var links []NetworkLink
	var out []NetworkLink

	id := "(network) (links)"

	i.g.Log.Debug(fmt.Sprintf("%s %s getting network links, type:'%s' prefix:'%s'",
		utils.GetGPid(), id, LinkType, LinkPrefix))

	// getting all links and filtering later
	if links, err = i.getNetworkLinks(ObjectsSource); err != nil {
		return out, err
	}

	i.g.Log.Debug(fmt.Sprintf("%s %s kernel returns links count:'%d'",
		utils.GetGPid(), id, len(links)))

	for _, l := range links {
		if strings.HasPrefix(l.Ifname, LinkPrefix) &&
			l.LinkType == LinkType {

			// Setting all flags in tunnel as UP if UP
			// present or DOWN if DOWN present
			if utils.StringInSlice(LINKFLAG_UP, l.Flags) {
				l.Flags = []string{LINKFLAG_UP}
			}
			if utils.StringInSlice(LINKFLAG_DOWN, l.Flags) {
				l.Flags = []string{LINKFLAG_DOWN}
			}
			out = append(out, l)
		}
	}

	i.g.Log.Debug(fmt.Sprintf("%s %s filtered links count:'%d' -> '%d'",
		utils.GetGPid(), id, len(links), len(out)))

	return out, err
}

const (
	PREFIXTYPE_MPLS   = "mpls"
	PREFIXTYPE_NATIVE = "native"
)

const (
	ROUTE_IP4 = 1001
	ROUTE_IP6 = 1002

	ROUTE_IP4_STR = "ip4"
	ROUTE_IP6_STR = "ip6"

	ROUTE_IP4_CONST = "-4"
	ROUTE_IP6_CONST = "-6"
)

func RouteAsString(route int) string {
	routes := map[int]string{
		ROUTE_IP4: ROUTE_IP4_STR,
		ROUTE_IP6: ROUTE_IP6_STR,
	}
	return routes[route]
}

func RouteAsConst(route int) string {
	consts := map[int]string{
		ROUTE_IP4: ROUTE_IP4_CONST,
		ROUTE_IP6: ROUTE_IP6_CONST,
	}
	return consts[route]
}

const (

	// bgp session delivers a prefixed path with some
	// specific prefix in a form 185.106.104.161/32, we
	// need to convert it to default for tunnles mode
	ROUTE_DEFAULT = "default"

	ROUTE_NOSPECIFICGATEWAY = ""

	ROUTE_NOSPECIFICDEVICE = ""
)

// we need a list of route types to set for
// tunnels scheme and for trasnsit
const (
	// tunnels mode with mpls encoding label
	ROUTETYPE_TUNNELS_MPLS = 1000

	// tunnels mode default via dev?
	ROUTETYPE_TUNNELS_DEFAULT = 1001

	// route type is mpls table object
	// with a list of ecmp routes to
	// dev for different tunnels
	ROUTETYPE_TUNNELS_ECMP_MPLS = 1002

	// route type for border loopbacks
	// need to set specific mtu and advmss
	// could be only ip6
	ROUTETYPE_TUNNELS_LOOPBACK_STATIC = 1003

	// transit mode with mpls encoding label via gateways
	ROUTETYPE_TRANSITS_MPLS = 2000

	// transit mode default via gateways
	ROUTETYPE_TRANSITS_DEFAULT = 2001

	// we need detect some defaults to
	// generate route bypass
	ROUTETYPE_BYPASS_TUNNELS_DEFAULT = 3000

	// one more case for transits default ip4
	ROUTETYPE_BYPASS_IP4_DEFAULT = 3001

	ROUTETYPE_UNKNOWN = 0
)

func RouteTypeAsString(rt int) string {
	rts := map[int]string{
		ROUTETYPE_TUNNELS_MPLS:            "rt+tunnels+mpls",
		ROUTETYPE_TUNNELS_DEFAULT:         "rt+tunnels+default",
		ROUTETYPE_TUNNELS_ECMP_MPLS:       "rt+tunnels+mpls+ecmp",
		ROUTETYPE_TUNNELS_LOOPBACK_STATIC: "rt+tunnels+loopback",
		ROUTETYPE_TRANSITS_MPLS:           "rt+transits+mpls",
		ROUTETYPE_TRANSITS_DEFAULT:        "rt+transits+default",
		ROUTETYPE_BYPASS_TUNNELS_DEFAULT:  "rt+bypass+tunnels",
		ROUTETYPE_BYPASS_IP4_DEFAULT:      "rt+ip4+default",
		ROUTETYPE_UNKNOWN:                 "rt+unknown",
	}

	if _, ok := rts[rt]; !ok {
		return rts[ROUTETYPE_UNKNOWN]
	}
	return rts[rt]
}

// ip ecmp route object for tunnels scheme,
// the rest attributes are derived from parent
// NetworkRoute object, such as: prefix, metric?
type NetworkEcmpRoute struct {
	// optional mpls label for PREFIXTYPE_MPLS
	Label uint32 `json:"label"`

	// device
	Device string `json:"device"`

	// weight
	Weight uint32 `json:"weight"`

	// table
	Table uint32 `json:"table"`
}

func (n NetworkEcmpRoute) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("table:'%d'", n.Table))
	out = append(out, fmt.Sprintf("label:'%d'", n.Label))
	out = append(out, fmt.Sprintf("device:'%s'", n.Device))
	out = append(out, fmt.Sprintf("weight:'%d'", n.Weight))

	return fmt.Sprintf("%s", strings.Join(out, ", "))
}

// ip route object
type NetworkRoute struct {
	// could be "default", "local", or native prefix
	Prefix string `json:"prefix"`

	// could be one of PREFIXTYPE_MPLS, PREFIXTYPE_NATIVE
	//PrefixType string `json:"prefix-type"`

	// optional mpls label for PREFIXTYPE_MPLS
	Label uint32 `json:"label"`

	// optional gateway
	Gateway string `json:"gateway"`

	// device
	Device string `json:"device"`

	// table
	Table uint32 `json:"table"`

	// Afi
	Afi int `json:"afi"`

	// For transit and tunnel mpls scheme we need
	// metric mtu advmss
	Mtu    uint32 `json:"mtu"`
	Advmss uint32 `json:"advmss"`
	Metric uint32 `json:"metric'`

	// It could be proto xorp for transits
	// and empty (not use for tunnels)
	Proto string `json:"proto"`

	RouteType int `json:"route-type"`

	// Ecmp routes linked to a table, should be
	// empty for all types of routes besides
	// ROUTETYPE_TUNNELS_ECMP_MPLS
	EcmpRoutes []NetworkEcmpRoute `json:"ecmp-routes,omitempty"`
}

const (
	STRUCTURE_O_ADD = "add"
	STRUCTURE_O_DEL = "del"
)

const (
	ECMP_MPLS_METRIC_ROUTE = 1024
)

func (n NetworkRoute) Structure(operation string) []string {
	var args []string

	if n.RouteType == ROUTETYPE_TUNNELS_LOOPBACK_STATIC {
		args = []string{RouteAsConst(n.Afi), "route", operation,
			n.Prefix, "dev", n.Device, "via", n.Gateway, "mtu",
			fmt.Sprintf("%d", n.Mtu), "advmss",
			fmt.Sprintf("%d", n.Advmss)}
	}

	if n.RouteType == ROUTETYPE_TUNNELS_MPLS {

		if n.Mtu > 0 && n.Advmss > 0 {
			args = []string{RouteAsConst(n.Afi), "route", operation,
				n.Prefix, "encap", "mpls", fmt.Sprintf("%d", n.Label),
				"dev", n.Device, "mtu", fmt.Sprintf("%d", n.Mtu),
				"advmss", fmt.Sprintf("%d", n.Advmss),
				"table", fmt.Sprintf("%d", n.Table)}
		} else {
			args = []string{RouteAsConst(n.Afi), "route", operation,
				n.Prefix, "encap", "mpls", fmt.Sprintf("%d", n.Label),
				"dev", n.Device, "table", fmt.Sprintf("%d", n.Table)}
		}
	}

	if n.RouteType == ROUTETYPE_TUNNELS_ECMP_MPLS {

		// ip -4 route replace default metric 1024 pref medium table 1523000 \
		//      nexthop encap mpls 182 dev strm-7bbff weight 1 \
		//      nexthop encap mpls 22016 dev strm-beebf weight 2

		if n.Mtu > 0 && n.Advmss > 0 {
			args = []string{RouteAsConst(n.Afi), "route", operation,
				n.Prefix, "metric", fmt.Sprintf("%d", ECMP_MPLS_METRIC_ROUTE),
				"mtu", fmt.Sprintf("%d", n.Mtu),
				"advmss", fmt.Sprintf("%d", n.Advmss),
				"table", fmt.Sprintf("%d", n.Table)}
		} else {
			args = []string{RouteAsConst(n.Afi), "route", operation,
				n.Prefix, "metric", fmt.Sprintf("%d", ECMP_MPLS_METRIC_ROUTE),
				"table", fmt.Sprintf("%d", n.Table)}
		}

		for _, e := range n.EcmpRoutes {
			route := []string{"nexthop", "encap", "mpls", fmt.Sprintf("%d", e.Label),
				"dev", fmt.Sprintf("%s", e.Device),
				"weight", fmt.Sprintf("%d", e.Weight)}
			args = append(args, route...)
		}
	}

	if n.RouteType == ROUTETYPE_TUNNELS_DEFAULT {
		args = []string{RouteAsConst(n.Afi), "route", operation,
			n.Prefix, "dev", n.Device, "via", n.Gateway, "mtu", fmt.Sprintf("%d", n.Mtu),
			"advmss", fmt.Sprintf("%d", n.Advmss), "table",
			fmt.Sprintf("%d", n.Table)}
	}

	if n.RouteType == ROUTETYPE_TRANSITS_MPLS {
		args = []string{RouteAsConst(n.Afi), "route", operation,
			n.Prefix, "encap", "mpls", fmt.Sprintf("%d", n.Label),
			"via", n.Gateway, "mtu", fmt.Sprintf("%d", n.Mtu),
			"advmss", fmt.Sprintf("%d", n.Advmss), "table",
			fmt.Sprintf("%d", n.Table), "proto", n.Proto, "metric",
			fmt.Sprintf("%d", n.Metric)}
	}

	if n.RouteType == ROUTETYPE_TRANSITS_DEFAULT {
		args = []string{RouteAsConst(n.Afi), "route", operation,
			n.Prefix, "via", n.Gateway, "mtu", fmt.Sprintf("%d", n.Mtu),
			"advmss", fmt.Sprintf("%d", n.Advmss), "table",
			fmt.Sprintf("%d", n.Table), "proto", n.Proto, "metric",
			fmt.Sprintf("%d", n.Metric)}
	}

	return args
}

func (n NetworkRoute) EcmpRoutesAsString() string {
	var routes []string

	sort.Slice(n.EcmpRoutes, func(i, j int) bool {
		return n.EcmpRoutes[i].Device < n.EcmpRoutes[j].Device
	})

	for i, e := range n.EcmpRoutes {
		routes = append(routes, fmt.Sprintf("[%d]/[%d] %s",
			i, len(n.EcmpRoutes), e.AsString()))
	}
	return fmt.Sprintf("%s", strings.Join(routes, ","))
}

func (n NetworkRoute) Create(g *config.CmdGlobal,
	ObjectsSource *TObjectsSource) error {
	var err error
	if _, err = GenericExec(g, ObjectsSource,
		n.Structure(STRUCTURE_O_ADD), "route",
		O_CREATE); err != nil {
		return err
	}
	return err
}

func (n NetworkRoute) Remove(g *config.CmdGlobal,
	ObjectsSource *TObjectsSource) error {
	var err error
	if _, err = GenericExec(g, ObjectsSource,
		n.Structure(STRUCTURE_O_DEL), "route",
		O_REMOVE); err != nil {
		return err
	}
	return err
}

func (n NetworkRoute) Key() string {

	var out []string

	out = append(out, fmt.Sprintf("%s", RouteAsString(n.Afi)))

	out = append(out, fmt.Sprintf("%s", n.Prefix))
	out = append(out, fmt.Sprintf("%d", n.RouteType))
	if n.RouteType == ROUTETYPE_TUNNELS_MPLS || n.RouteType == ROUTETYPE_TRANSITS_MPLS {
		out = append(out, fmt.Sprintf("%d", n.Label))
	}
	if len(n.Gateway) > 0 {
		out = append(out, fmt.Sprintf("%s", n.Gateway))
	}
	out = append(out, fmt.Sprintf("%s", n.Device))
	out = append(out, fmt.Sprintf("%d", n.Table))

	if n.RouteType == ROUTETYPE_TUNNELS_ECMP_MPLS {
		out = append(out, fmt.Sprintf("ecmp: %s",
			n.EcmpRoutesAsString()))
	}

	return fmt.Sprintf("%s", strings.Join(out, "-"))
}

func (n NetworkRoute) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("afi:'%s'", RouteAsString(n.Afi)))

	out = append(out, fmt.Sprintf("prefix:'%s'", n.Prefix))
	out = append(out, fmt.Sprintf("route-type:'%s'",
		RouteTypeAsString(n.RouteType)))

	if n.RouteType == ROUTETYPE_TUNNELS_MPLS || n.RouteType == ROUTETYPE_TRANSITS_MPLS {
		out = append(out, fmt.Sprintf("label:'%d'", n.Label))
	}
	if len(n.Gateway) > 0 {
		out = append(out, fmt.Sprintf("gateway:'%s'", n.Gateway))
	}

	if n.RouteType == ROUTETYPE_TUNNELS_MPLS ||
		n.RouteType == ROUTETYPE_BYPASS_TUNNELS_DEFAULT ||
		n.RouteType == ROUTETYPE_TUNNELS_LOOPBACK_STATIC {
		out = append(out, fmt.Sprintf("device:'%s'", n.Device))

		// Mtu and advmss could be optional
		if n.Mtu > 0 && n.Advmss > 0 {
			out = append(out, fmt.Sprintf("mtu:'%d'", n.Mtu))
			out = append(out, fmt.Sprintf("advmss:'%d'", n.Advmss))
		}
	}

	if n.Table > 0 {
		out = append(out, fmt.Sprintf("table:'%d'", n.Table))
	}

	if n.RouteType == ROUTETYPE_TUNNELS_ECMP_MPLS {

		// Mtu and advmss could be optional
		if n.Mtu > 0 && n.Advmss > 0 {
			out = append(out, fmt.Sprintf("mtu:'%d'", n.Mtu))
			out = append(out, fmt.Sprintf("advmss:'%d'", n.Advmss))
		}

		out = append(out, fmt.Sprintf("ecmp: %s",
			n.EcmpRoutesAsString()))
	}

	if n.RouteType == ROUTETYPE_TRANSITS_MPLS ||
		n.RouteType == ROUTETYPE_TRANSITS_DEFAULT ||
		n.RouteType == ROUTETYPE_BYPASS_TUNNELS_DEFAULT {
		out = append(out, fmt.Sprintf("mtu:'%d'", n.Mtu))
		out = append(out, fmt.Sprintf("advmss:'%d'", n.Advmss))
		out = append(out, fmt.Sprintf("metric:'%d'", n.Metric))

		out = append(out, fmt.Sprintf("proto:'%s'", n.Proto))
	}

	return fmt.Sprintf("%s", strings.Join(out, ", "))
}

// Getting routes (it could be mpls based or native) for
// proto specified
func (i Ip) getSpecificNetworkRoutes(ObjectsSource *TObjectsSource,
	proto string) ([]NetworkRoute, error) {

	var err error
	var out []NetworkRoute

	class := i.g.Opts.Runtime.NetworkClass
	loopbacks := i.g.Opts.Network.Tunnels.TunnelsRoutes

	families := []int{ROUTE_IP4, ROUTE_IP6}
	for _, f := range families {
		c := RouteAsConst(f)

		args := []string{c, "route", "show", "table",
			"all", "proto", proto}

		var output []byte
		if output, err = GenericExec(i.g, ObjectsSource, args, "show", "routes"); err != nil {
			i.g.Log.Error(fmt.Sprintf("%s error executing command, err:'%s'",
				utils.GetGPid(), err))
			return out, err
		}

		i.g.LogDump(utils.GetGPid(), string(output))

		rows := strings.Split(string(output), "\n")
		j := 0
		for j < len(rows) {
			r := rows[j]
			j++

			// Assuming that encap mpls is following first
			t := strings.TrimRight(r, " \t")
			if len(t) == 0 {
				continue
			}

			var route NetworkRoute

			if loopbacks.Enabled {

				// 2a02:6b8:0:1400::be via 2a02:6b8:b010:a4fc::1 dev eth0 metric 1024 mtu 8840 advmss 8900 pref medium
				regexps := []string{
					`^(.*)\s+via\s+(.*)\s+dev\s+(.*)\s+metric\s+(.*)\s+mtu\s+(.*)\s+advmss\s+(.*)$`,
				}

				for _, r := range regexps {
					re := regexp.MustCompile(r)
					s := re.FindStringSubmatch(t)

					if len(s) < 4 {
						continue
					}

					rt := ROUTETYPE_TUNNELS_LOOPBACK_STATIC
					if (s[2] != loopbacks.GatewayIp6Value) && (s[3] != loopbacks.DeviceValue) {
						continue
					}
					if strings.Contains(s[1], "::/") || strings.Contains(s[1], "default") {
						continue
					}

					if route, err = i.ParseRoute(s, rt, f); err != nil {
						i.g.Log.Debug(fmt.Sprintf("%s error parsing route, row:'%s' err:'%s'",
							utils.GetGPid(), t, err))
						continue
					}
					out = append(out, route)

					i.g.Log.Debug(fmt.Sprintf("%s '%s' loopbacks len:'%d' row '%s' parsed -> '[%s]' ",
						utils.GetGPid(), RouteAsString(f), len(s), s,
						strings.Join(s, ",")))
				}
			}

			// default routes to detect in transit mode xorp
			if !strings.Contains(t, "mpls") && (class == config.NETWORK_CLASS_TRANSITS ||
				class == config.NETWORK_CLASS_TUNNELS) {

				// TUNNELS do not have advmss?
				regexps := []string{
					`^(.*)\s+via\s+(.*)\s+dev\s+(.*)\s+table\s+(.*)\s+metric\s+(.*)\s+mtu\s+(.*)\s+advmss\s+(.*)\s+pref\s+(.*)$`,
					`^(.*)\s+via\s+(.*)\s+dev\s+(.*)\s+table\s+(.*)\s+metric\s+(.*)\s+mtu\s+(.*)\s+advmss\s+(.*)$`,
					`^(.*)\s+via\s+(.*)\s+dev\s+(.*)\s+table\s+(.*)\s+metric\s+(.*)\s+mtu\s+(.*)\s+pref\s+(.*)$`,
					`^(.*)\s+via\s+(.*)\s+dev\s+(.*)\s+table\s+(.*)\s+metric\s+(.*)\s+mtu\s+(.*)$`,
				}

				for _, r := range regexps {
					re := regexp.MustCompile(r)
					s := re.FindStringSubmatch(t)
					if len(s) == 0 {
						continue
					}
					i.g.Log.Debug(fmt.Sprintf("%s '%s' len:'%d' default+gw row '%s' parsed -> '[%s]' ",
						utils.GetGPid(), RouteAsString(f), len(s), s,
						strings.Join(s, ",")))
					rt := ROUTETYPE_TRANSITS_DEFAULT
					if class == config.NETWORK_CLASS_TUNNELS {
						rt = ROUTETYPE_TUNNELS_DEFAULT
					}
					if route, err = i.ParseRoute(s, rt, f); err != nil {
						i.g.Log.Debug(fmt.Sprintf("%s error parsing route, row:'%s' err:'%s'",
							utils.GetGPid(), t, err))
						continue
					}

					// Some routes for VS for example we need skip
					if route.Table != 200 && route.Table != 201 {
						out = append(out, route)
					}
					break
				}
			}

			// mpls + gateway specific + ipv6 (has pref), ipv4 (not)
			if strings.Contains(t, "mpls") && strings.Contains(t, "via") {

				regexps := []string{
					`^(.*)\s+encap\s+mpls\s+(.*)\s+via\s+(.*)\s+dev\s+(.*)\s+table\s+(.*)\s+metric\s+(.*)\s+mtu\s+(.*)\s+advmss\s+(.*)\s+pref\s+(.*)$`,
					`^(.*)\s+encap\s+mpls\s+(.*)\s+via\s+(.*)\s+dev\s+(.*)\s+table\s+(.*)\s+metric\s+(.*)\s+mtu\s+(.*)\s+advmss\s+(.*)$`,
				}

				for _, r := range regexps {
					re := regexp.MustCompile(r)
					s := re.FindStringSubmatch(t)
					if len(s) == 0 {
						continue
					}
					i.g.Log.Debug(fmt.Sprintf("%s '%s' len:'%d' mpls+gw row '%s' parsed -> '[%s]' ",
						utils.GetGPid(), RouteAsString(f), len(s), s,
						strings.Join(s, ",")))
					if route, err = i.ParseRoute(s, ROUTETYPE_TRANSITS_MPLS, f); err != nil {
						i.g.Log.Debug(fmt.Sprintf("%s error parsing route, row:'%s' err:'%s'",
							utils.GetGPid(), t, err))
						continue
					}
					out = append(out, route)
					break
				}
			}

			// mpls + dev
			if strings.Contains(t, "mpls") && !strings.Contains(t, "via") {
				re := regexp.MustCompile(`^(.*)\s+encap\s+mpls\s+(.*)\sdev\s+(.*)\s+table\s+(.*)\s+(metric|scope)\s+(.*)$`)
				s := re.FindStringSubmatch(t)

				// mpls ecmp has similiar signature
				if len(s) == 0 {
					continue
				}

				i.g.Log.Debug(fmt.Sprintf("%s '%s' len:'%d' mpls row '%s' parsed -> '[%s]'",
					utils.GetGPid(), RouteAsString(f), len(s), s,
					strings.Join(s, ",")))
				if route, err = i.ParseRoute(s, ROUTETYPE_TUNNELS_MPLS, f); err != nil {
					i.g.Log.Error(fmt.Sprintf("%s error parsing route, row:'%s' err:'%s'",
						utils.GetGPid(), t, err))
					continue
				}

				out = append(out, route)
			}

			// ecmp routes in table defined as following template
			// default table 1523000 metric 1024
			//	nexthop  encap mpls  182 dev strm-7bbff weight 1
			//	nexthop  encap mpls  22016 dev strm-beebf weight 2
			re := regexp.MustCompile(`^(default|local)\s+table\s+(.*)\s+metric\s+(.*)$`)
			s := re.FindStringSubmatch(t)
			if len(s) > 0 {
				i.g.Log.Debug(fmt.Sprintf("%s '%s' len:'%d' ecmp mpls row '%s' parsed -> '[%s]'",
					utils.GetGPid(), RouteAsString(f), len(s), s,
					strings.Join(s, ",")))

				if len(s) < 4 {
					continue
				}

				// Detecting table id
				ttable := s[2]
				var table uint64
				if table, err = strconv.ParseUint(ttable, 10, 64); err != nil {
					i.g.Log.Error(fmt.Sprintf("%s error converting table:'%s' to uint32, err:'%s'",
						utils.GetGPid(), ttable, err))
					continue
				}
				route.Table = uint32(table)
				route.Prefix = "default"
				route.Afi = f
				route.RouteType = ROUTETYPE_TUNNELS_ECMP_MPLS

				re := regexp.MustCompile(`^\s+nexthop\s+encap\s+mpls\s+(.*)\s+dev\s+(.*)\s+weight\s+(.*)$`)
				base := j
				for k := base; k < len(rows); k++ {
					j++
					t := rows[k]
					s := re.FindStringSubmatch(t)
					if len(s) == 0 {
						j--
						break
					}
					i.g.Log.Debug(fmt.Sprintf("%s '%s' len:'%d' ecmp mpls child [%d]/[%d]:[%d] row '%s' parsed -> '[%s]'",
						utils.GetGPid(), RouteAsString(f), len(s), k, len(rows), k-base, s,
						strings.Join(s, ",")))

					var eroute NetworkEcmpRoute
					if eroute, err = i.ParseEcmpRoute(s); err != nil {
						i.g.Log.Error(fmt.Sprintf("%s error parsing ecmp route, row:'%s' err:'%s'",
							utils.GetGPid(), t, err))
						continue
					}
					eroute.Table = route.Table
					i.g.Log.Debug(fmt.Sprintf("%s '%s' ecmp route parsed '%s'", utils.GetGPid(),
						RouteAsString(f), eroute.AsString()))
					route.EcmpRoutes = append(route.EcmpRoutes, eroute)
				}

				out = append(out, route)
			}
		}
	}

	return out, err
}

func (i *Ip) ParseEcmpRoute(tags []string) (NetworkEcmpRoute, error) {
	var route NetworkEcmpRoute
	var err error

	min := 4
	if len(tags) < min {
		err = errors.New(fmt.Sprintf("error, tags count less than minimal"))
		i.g.Log.Debug(fmt.Sprintf("%s count:'%d' minimum:'%d', err:'%s'",
			utils.GetGPid(), len(tags), min, err))
		return route, err
	}

	tlabel := strings.Trim(tags[1], " ")
	tdevice := strings.Trim(tags[2], " ")
	tweight := strings.Trim(tags[3], " ")

	var label uint64
	if label, err = strconv.ParseUint(tlabel, 10, 64); err != nil {
		i.g.Log.Error(fmt.Sprintf("%s %s error converting label:'%s' to uint32, err:'%s'",
			utils.GetGPid(), tlabel, err))
		return route, err
	}
	route.Label = uint32(label)
	route.Device = tdevice

	var weight uint64
	if weight, err = strconv.ParseUint(tweight, 10, 64); err != nil {
		i.g.Log.Error(fmt.Sprintf("%s %s error converting weight:'%s' to uint32, err:'%s'",
			utils.GetGPid(), tweight, err))
		return route, err
	}
	route.Weight = uint32(weight)

	return route, err
}

const (
	TAGS_COUNT_MPLS_DEV     = 4
	TAGS_COUNT_MPLS_GATEWAY = 6
)

// Assuming that tags is an array from regexp matching
func (i *Ip) ParseRoute(tags []string, rt int, afi int) (NetworkRoute, error) {
	var err error

	var route NetworkRoute
	route.Afi = afi
	route.RouteType = rt

	min := TAGS_COUNT_MPLS_DEV
	if len(tags) < min {
		err = errors.New(fmt.Sprintf("error, tags:['%s'] count less than minimal",
			strings.Join(tags, ",")))
		i.g.Log.Debug(fmt.Sprintf("%s count:'%d' minimum:'%d', err:'%s'",
			utils.GetGPid(), len(tags), min, err))
		return route, err
	}

	route.Prefix = tags[1]
	route.Prefix = strings.Trim(route.Prefix, " ")

	var ttable string

	tlabel := strings.Trim(tags[2], " ")

	if rt == ROUTETYPE_BYPASS_TUNNELS_DEFAULT {
		// 2a02:6b8:b010:a4fc::1,eth0,1024,1450,1390,medium
		route.Prefix = "default"
		route.Gateway = tags[1]
		route.Device = tags[2]
	}

	if rt == ROUTETYPE_BYPASS_IP4_DEFAULT {
		// 5.45.192.254,eth0,1450,1410
		route.Prefix = "default"
		route.Gateway = tags[1]
		route.Device = tags[2]

		return route, err
	}

	if rt == ROUTETYPE_TUNNELS_LOOPBACK_STATIC {
		route.Gateway = tags[2]
		route.Device = tags[3]

		return route, err
	}

	// Device tag should not be set for ROUTETYPE_TRANSITS_DEFAULT
	if rt == ROUTETYPE_TUNNELS_MPLS || rt == ROUTETYPE_TUNNELS_DEFAULT {
		route.Device = tags[3]
		ttable = tags[4]
	}

	if rt == ROUTETYPE_TRANSITS_MPLS || rt == ROUTETYPE_TRANSITS_DEFAULT ||
		rt == ROUTETYPE_TUNNELS_DEFAULT || rt == ROUTETYPE_BYPASS_TUNNELS_DEFAULT {
		offset := 3
		if rt == ROUTETYPE_TRANSITS_DEFAULT || rt == ROUTETYPE_TUNNELS_DEFAULT {
			offset = 2
		}

		if rt == ROUTETYPE_TRANSITS_MPLS || rt == ROUTETYPE_TRANSITS_DEFAULT ||
			rt == ROUTETYPE_TUNNELS_DEFAULT {
			route.Gateway = tags[offset]
			ttable = tags[offset+2]
		}

		if rt == ROUTETYPE_BYPASS_TUNNELS_DEFAULT {
			offset = 0
		}
		var vv uint64

		tvalue := tags[offset+3]
		if vv, err = strconv.ParseUint(tvalue, 10, 64); err != nil {
			i.g.Log.Error(fmt.Sprintf("%s %s error converting metric:'%s' to uint32, err:'%s'",
				utils.GetGPid(), RouteTypeAsString(rt), tvalue, err))
			return route, err
		}
		route.Metric = uint32(vv)

		tvalue = tags[offset+4]
		if vv, err = strconv.ParseUint(tvalue, 10, 64); err != nil {
			i.g.Log.Error(fmt.Sprintf("%s %s error converting mtu:'%s' to uint32, err:'%s'",
				utils.GetGPid(), RouteTypeAsString(rt), tvalue, err))
			return route, err
		}
		route.Mtu = uint32(vv)

		// tunnels default routes do not have advmss
		if rt != ROUTETYPE_TUNNELS_DEFAULT {
			tvalue = tags[offset+5]
			if vv, err = strconv.ParseUint(tvalue, 10, 64); err != nil {
				i.g.Log.Error(fmt.Sprintf("%s %s error converting advmss:'%s' to uint32, err:'%s'",
					utils.GetGPid(), RouteTypeAsString(rt), tvalue, err))
				return route, err
			}
			route.Advmss = uint32(vv)
		}

		route.Proto = i.g.Opts.Network.Transits.Proto
		if rt == ROUTETYPE_BYPASS_TUNNELS_DEFAULT {
			route.Proto = i.g.Opts.Network.Tunnels.Proto
		}
	}

	if rt == ROUTETYPE_TRANSITS_MPLS || rt == ROUTETYPE_TUNNELS_MPLS {
		var label uint64
		if label, err = strconv.ParseUint(tlabel, 10, 64); err != nil {
			i.g.Log.Error(fmt.Sprintf("%s %s error converting label:'%s' to uint32, err:'%s'",
				utils.GetGPid(), RouteTypeAsString(rt), tags[2], err))
			return route, err
		}
		route.Label = uint32(label)
	}

	if rt != ROUTETYPE_BYPASS_TUNNELS_DEFAULT {
		var table uint64
		if table, err = strconv.ParseUint(ttable, 10, 64); err != nil {
			i.g.Log.Error(fmt.Sprintf("%s %s error converting table:'%s' to uint32, err:'%s'",
				utils.GetGPid(), RouteTypeAsString(rt), ttable, err))
			return route, err
		}
		route.Table = uint32(table)
	}

	return route, err
}

// ip rule object
type NetworkRule struct {

	// Afi:  ROUTE_IP4, ROUTE_IP6
	Afi int `json:"afi"`

	// table
	Table uint32 `json:"table"`

	// fwmark (0x5f3, 0x5f9)
	Mark uint32 `json:"mark"`
}

func (n NetworkRule) Key() string {
	return fmt.Sprintf("%d-%d-%d", n.Afi, n.Table, n.Mark)
}

func (n NetworkRule) Create(g *config.CmdGlobal, ObjectsSource *TObjectsSource) error {
	var err error
	args := []string{RouteAsConst(n.Afi), "rule", "add",
		"fwmark", fmt.Sprintf("0x%0x", n.Mark), "lookup",
		fmt.Sprintf("%d", n.Table)}
	if _, err = GenericExec(g, ObjectsSource, args, "rule", O_CREATE); err != nil {
		return err
	}

	return err
}

func (n NetworkRule) Remove(g *config.CmdGlobal, ObjectsSource *TObjectsSource) error {
	var err error

	args := []string{RouteAsConst(n.Afi), "rule", "del",
		"fwmark", fmt.Sprintf("0x%0x", n.Mark), "lookup",
		fmt.Sprintf("%d", n.Table)}
	if _, err = GenericExec(g, ObjectsSource, args, "rule", O_CREATE); err != nil {
		return err
	}

	return err
}

func (n NetworkRule) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("afi:'%s'", RouteAsString(n.Afi)))
	out = append(out, fmt.Sprintf("table:'%d'", n.Table))
	out = append(out, fmt.Sprintf("fwmark:'0x%0x'", n.Mark))

	return fmt.Sprintf("%s", strings.Join(out, ", "))
}

const (
	TAGS_COUNT_FWMARK = 4
)

// Assuming that tags is an array from regexp matching
func (i *Ip) ParseRule(tags []string, afi int) (NetworkRule, error) {
	var err error
	var rule NetworkRule

	min := TAGS_COUNT_MPLS_DEV
	if len(tags) < min {
		i.g.Log.Error(fmt.Sprintf("%s error, tags count:'%d' less than minimum:'%d' possible",
			utils.GetGPid(), len(tags), min))
		return rule, err
	}

	// 32764:,0x5f9,1529000
	var table uint64
	if table, err = strconv.ParseUint(tags[3], 10, 64); err != nil {
		i.g.Log.Error(fmt.Sprintf("%s error converting table:'%s' to uint32, err:'%s'",
			utils.GetGPid(), tags[3], err))
		return rule, err
	}
	rule.Table = uint32(table)

	// Need convert 0x number into number
	var mark uint64
	if mark, err = strconv.ParseUint(strings.TrimLeft(tags[2], "0x"), 16, 64); err != nil {
		i.g.Log.Error(fmt.Sprintf("%s error converting mark:'%s' to uint32, err:'%s'",
			utils.GetGPid(), tags[2], err))
		return rule, err
	}

	rule.Mark = uint32(mark)
	rule.Afi = afi

	return rule, err
}

// Getting rules with fwmarks
func (i *Ip) getFwmarkNetworkRules(ObjectsSource *TObjectsSource) ([]NetworkRule, error) {
	var err error
	var out []NetworkRule

	families := []int{ROUTE_IP4, ROUTE_IP6}
	for _, f := range families {
		c := RouteAsConst(f)

		args := []string{c, "rule", "list"}

		var output []byte
		if output, err = GenericExec(i.g, ObjectsSource, args, "show", "rules"); err != nil {
			i.g.Log.Error(fmt.Sprintf("%s error executing command, err:'%s'",
				utils.GetGPid(), err))
			return out, err
		}

		rows := strings.Split(string(output), "\n")
		for _, r := range rows {

			// Assuming that encap mpls is following first
			t := strings.TrimRight(r, " \t")
			if len(t) == 0 {
				continue
			}

			var rule NetworkRule
			// mpls + gateway specific
			if strings.Contains(t, "fwmark") {
				re := regexp.MustCompile(`^(.*)\s+from\s+all\s+fwmark\s+(.*)\s+lookup\s+(.*)`)
				s := re.FindStringSubmatch(t)
				i.g.Log.Debug(fmt.Sprintf("%s '%s' len:'%d' fwmark rule '%s' parsed -> '[%s]'",
					utils.GetGPid(), RouteAsString(f), len(s), s,
					strings.Join(s, ",")))

				if rule, err = i.ParseRule(s, f); err != nil {
					i.g.Log.Error(fmt.Sprintf("%s error parsing rule, row:'%s' err:'%s'",
						utils.GetGPid(), t, err))
					continue
				}

				// TODO: we need filter rule for static fwmark+vlan
				// static rules defined as static matchers from
				// network/interfaces

				min := uint32(1500)
				max := uint32(1999)
				if rule.Table > min && rule.Table < max {
					// skipping fwmark rule
					continue
				}
				out = append(out, rule)
			}
		}
	}
	return out, err
}

// network objects linked to some namespace or
// container if control is from dom0
type TObjects struct {

	// optional container pointer
	Container *lxd.Container

	Links  []NetworkLink  `json:"links"`
	Rules  []NetworkRule  `json:"rules"`
	Routes []NetworkRoute `json:"routes"`
}

func (t *TObjects) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("links:'%d'", len(t.Links)))
	out = append(out, fmt.Sprintf("rules:'%d'", len(t.Rules)))
	out = append(out, fmt.Sprintf("routes:'%d'", len(t.Routes)))

	return fmt.Sprintf("%s", strings.Join(out, ","))
}

func (t *TObjects) DumpObjects(g *config.CmdGlobal) {
	id := "(dump)"
	for i, l := range t.Links {
		g.Log.Info(fmt.Sprintf("%s %s [%d]/[%d] link:'%s'",
			utils.GetGPid(), id, i, len(t.Links), l.AsString()))
	}

	for i, r := range t.Routes {
		g.Log.Info(fmt.Sprintf("%s %s [%d]/[%d] route:'%s'",
			utils.GetGPid(), id, i, len(t.Routes),
			r.AsString()))
	}

	for i, r := range t.Rules {
		g.Log.Info(fmt.Sprintf("%s %s [%d]/[%d] rule'%s'",
			utils.GetGPid(), id, i, len(t.Rules),
			r.AsString()))
	}
}

// Getting some overrides properties or default
func (i *Ip) getOverrides() config.TransitsOverride {
	transits := i.g.Opts.Network.Transits

	var out config.TransitsOverride
	out.GatewayIp4 = transits.GatewayIp4
	out.GatewayIp6 = transits.GatewayIp6

	loc := config.LocationAsString(i.g.Opts.Runtime.Location)

	if w, ok := transits.Overrides[loc]; ok {
		out.GatewayIp4 = w.GatewayIp4
		out.GatewayIp6 = w.GatewayIp6
	}

	return out
}

// In some scenarious we need automaitcally detection
// for default routes/devices/parameters of NetworkRoute
func (i *Ip) getDefaultRoute(ObjectsSource *TObjectsSource, afi int) (*NetworkRoute, error) {
	var err error
	var r NetworkRoute

	c := RouteAsConst(afi)
	id := "(default)"

	i.g.Log.Debug(fmt.Sprintf("%s %s %s request to detect default route",
		utils.GetGPid(), id, RouteAsString(afi)))

	args := []string{c, "route", "list", "default"}

	var output []byte
	if output, err = GenericExec(i.g, ObjectsSource, args, "show", "default"); err != nil {
		i.g.Log.Error(fmt.Sprintf("%s error executing command, err:'%s'",
			utils.GetGPid(), err))
		return &r, err
	}

	i.g.LogDump(utils.GetGPid(), string(output))

	var out []NetworkRoute
	rows := strings.Split(string(output), "\n")
	for j, r := range rows {
		t := strings.TrimRight(r, " \t")
		if len(t) == 0 {
			continue
		}

		var route NetworkRoute
		i.g.Log.Debug(fmt.Sprintf("%s [%d]/[%d] default route '%s'",
			utils.GetGPid(), j, len(rows), r))

		// mpls + gateway specific + ipv6 (has pref), ipv4 (not)
		if strings.Contains(t, "default") && strings.Contains(t, "via") {
			regexps := []string{
				`^default\s+via\s+(.*)\s+dev\s+(.*)\s+metric\s+(.*)\s+mtu\s+(.*)\s+advmss\s+(.*)\s+pref\s+(.*)$`,
				`^default\s+via\s+(.*)\s+dev\s+(.*)\s+mtu\s+(.*)\s+advmss\s+(.*)$`,
			}

			// default via 2a02:6b8:c20:22f::badc:ab1e dev veth proto static metric 1024 mtu 8910 pref medium
			// default via 2a02:6b8:b010:a4fc::1 dev eth0 metric 1024 mtu 1450 advmss 1390 pref medium
			// default dev tun0 scope link mtu 1450 advmss 1410

			// default via 37.9.93.158 dev eth0 mtu 1450 advmss 1410
			for _, r := range regexps {
				re := regexp.MustCompile(r)
				s := re.FindStringSubmatch(t)
				i.g.Log.Debug(fmt.Sprintf("%s '%s' len:'%d' default+gw row '%s' parsed -> '[%s]' ",
					utils.GetGPid(), RouteAsString(afi), len(s), s,
					strings.Join(s, ",")))
				if afi == ROUTE_IP6 {
					if route, err = i.ParseRoute(s, ROUTETYPE_BYPASS_TUNNELS_DEFAULT, afi); err != nil {
						i.g.Log.Debug(fmt.Sprintf("%s error parsing route, row:'%s' err:'%s'",
							utils.GetGPid(), t, err))
						continue
					}
				}
				if afi == ROUTE_IP4 {
					if route, err = i.ParseRoute(s, ROUTETYPE_BYPASS_IP4_DEFAULT, afi); err != nil {
						i.g.Log.Debug(fmt.Sprintf("%s error parsing route, row:'%s' err:'%s'",
							utils.GetGPid(), t, err))
						continue
					}
				}

				out = append(out, route)

				i.g.Log.Debug(fmt.Sprintf("%s '%s' route parsed as '%s'",
					utils.GetGPid(), RouteAsString(afi), route.AsString()))

				break
			}
		}
	}

	i.g.Log.Debug(fmt.Sprintf("%s default routes count:'%d' detected",
		utils.GetGPid(), len(out)))

	if len(out) == 0 {
		err = errors.New("no default route detected")
		return &r, err
	}

	r = out[0]
	return &r, err
}

type TEcmpNextHop struct {
	Label  uint32 `json:"label"`
	Device string `json:"device"`
	Weight uint32 `json:"weight"`
}

func (t *TEcmpNextHop) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("label:'%d'", t.Label))
	out = append(out, fmt.Sprintf("device:'%s'", t.Device))
	out = append(out, fmt.Sprintf("weight:'%d'", t.Weight))

	return fmt.Sprintf("%s", strings.Join(out, ","))
}

func NextHopsAsString(nexthops []TEcmpNextHop) string {
	var out []string
	for i, n := range nexthops {
		out = append(out, fmt.Sprintf("[%d]/[%d] %s",
			i, len(nexthops), n.AsString()))
	}
	return fmt.Sprintf("%s", strings.Join(out, ","))
}

const (
	// algorithm selection next hop w.r.t
	// linkid and device
	HASH_ALGO1     = 1001
	HASH_ALG01_STR = "hash:linkid"

	// algorithm selection w.r.t linkid and
	// fqdn of current host
	HASH_ALG02_STR = "hash:linkid+host"
	HASH_ALGO2     = 1002
)

// A data for hash to select nexthop as
// hash function from a list of variables
type THashData struct {
	linkid uint32
	fqdn   string
}

func (t *THashData) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("linkid:'%d'", t.linkid))
	out = append(out, fmt.Sprintf("fqdn:'%s'", t.fqdn))

	return fmt.Sprintf("%s", strings.Join(out, ","))
}

// Selecting nexthop from a list of possible variants
// w.r.t selection algorithm and its hashing function
func (i *Ip) nexthopHashSelect(h []TEcmpNextHop, data THashData) TEcmpNextHop {

	// Setting choosen algo, defaulting to algo2
	algostr := i.g.Opts.Network.Tunnels.EcmpIp6Algo
	algostrs := map[string]int{
		HASH_ALG01_STR: HASH_ALGO1,
		HASH_ALG02_STR: HASH_ALGO2,
	}
	algo := HASH_ALGO2
	if _, ok := algostrs[algostr]; ok {
		algo = algostrs[algostr]
	}

	sort.Slice(h, func(i, j int) bool {
		return h[i].Device < h[j].Device
	})

	var key string

	switch algo {
	case HASH_ALGO1:
		key = fmt.Sprintf("%d", data.linkid)
	case HASH_ALGO2:
		key = fmt.Sprintf("%d-%s", data.linkid, data.fqdn)
	}

	id := "(hash)"

	hash := sha1.Sum([]byte(key))
	hex := hex.EncodeToString(hash[:])

	buf := bytes.NewBuffer(hash[:])
	var number uint64
	if err := binary.Read(buf, binary.LittleEndian,
		&number); err != nil {
		i.g.Log.Debug(fmt.Sprintf("%s %s hash failed, err:'%s'",
			utils.GetGPid(), id, err))
		return h[0]
	}

	pos := number % uint64(len(h))

	i.g.Log.Debug(fmt.Sprintf("%s %s hash algo:'%s' -> '%d' hashdata:'%s' key:'%s' len:'%d' number:'%d' hash:'%s' -> pos:'%d'",
		utils.GetGPid(), id, algostr, algo, data.AsString(), key,
		len(h), number, hex, pos))

	return h[pos]
}

func (i *Ip) setMtu(r *NetworkRoute) {

	mplsroutes := i.g.Opts.Network.Tunnels.MplsRoutesMtu

	// Optional mtn and advmss configuration
	if mplsroutes.Enabled {
		switch r.Afi {
		case ROUTE_IP6:
			r.Mtu = mplsroutes.MtuDefaultIp6
			r.Advmss = mplsroutes.AdvmssDefaultIp6
		case ROUTE_IP4:
			r.Mtu = mplsroutes.MtuDefaultIp4
			r.Advmss = mplsroutes.AdvmssDefaultIp4
		}
	}
}

// Generating from route got from bgp corresponding objects
func (i *Ip) generateObjects(ObjectsSource *TObjectsSource,
	class int, paths []Path) (TObjects, error) {

	var err error
	var Objects TObjects

	id := "(generate)"

	i.g.Log.Debug(fmt.Sprintf("%s %s request to generate triple objects, class:'%s'",
		utils.GetGPid(), id, config.NetworkClassAsString(class)))

	// First, we need to map all data from neighbors into one version
	// he we assume that data is the same for all neighbors, if not
	// this is a problem
	prefixes := make(map[string]Path)
	neigbors := make(map[string][]string)

	// Second, there are several nexthops for ecmp versions,
	// so we just counting nexthops
	nexthops := make(map[string][]TEcmpNextHop)

	for _, p := range paths {
		linkid := p.Community.Linkid
		key := fmt.Sprintf("%d-%d-%s-%d-%s", p.Afi, p.Safi,
			p.Prefix, p.PrefixLen, linkid)
		prefixes[key] = p
		neigbors[key] = append(neigbors[key], p.Neighbor)

		if len(p.Labels) != 1 {
			i.g.Log.Error(fmt.Sprintf("%s %s error, path:'%s' has incorrect labels array count:'%d', skip it",
				utils.GetGPid(), id, p.AsMinimalString(), len(p.Labels)))
			continue
		}

		// AFI_IP6 should have only one nexthop
		// but which one and which policy is to use
		// but it the end it should be normal route
		var enh TEcmpNextHop
		enh.Device = p.HashNexthop()
		enh.Weight = 1
		enh.Label = p.Labels[0]

		// Checking if nexthops[key] TEcmpNextHop
		// alreay contains device or not
		contains := false
		for _, e := range nexthops[key] {
			if e.Device == enh.Device {
				contains = true
			}
		}
		if !contains {
			nexthops[key] = append(nexthops[key], enh)
		}
	}

	for k, p := range prefixes {
		n := neigbors[k]
		h := nexthops[k]
		i.g.Log.Debug(fmt.Sprintf("%s %s prefix %s <- [%d]:[%s] hash:nexthop:[%s] uniq nexthops:[%d]:['%s']",
			utils.GetGPid(), id, p.AsMinimalString(),
			len(n), strings.Join(n, ","), p.HashNexthop(),
			len(h), NextHopsAsString(h)))
	}

	// Generating links objects, we need to filter and detect a pair
	// of linkid -> nexthops

	rtm := i.g.Opts.Network.Tunnels.RouteTableMultiplier
	if class == config.NETWORK_CLASS_TUNNELS {

		prefix := i.g.Opts.Network.Tunnels.DevicePrefix
		local := i.g.Opts.Network.Tunnels.LocalAddressResolved
		dport := i.g.Opts.Network.Tunnels.EncapDestPort

		links := make(map[string]NetworkLink)
		count := make(map[string]int)

		for _, p := range prefixes {
			linkid := p.Community.Linkid

			for _, r := range p.NextHops {
				var l NetworkLink
				l.LinkType = LINK_TYPE_TUNNEL6
				l.Txqlen = TUNNEL6_TXQLEN
				l.Ifname = fmt.Sprintf("%s-%s", prefix, p.HashNexthop())
				l.Mtu = TUNNEL6_MTU
				l.Remote = r
				l.EncapDestPort = dport

				// Local address used as a local
				// endpoint for tunnel, is it possible
				// to detect it automatically? how
				// to filter all ip address to match
				// right one
				l.Local = local
				l.Flags = append(l.Flags, LINKFLAG_UP)

				key := fmt.Sprintf("%s-%d", l.Remote, linkid)

				links[key] = l
				count[key]++
			}
		}

		j := 0
		for k, l := range links {
			i.g.Log.Info(fmt.Sprintf("%s %s link [%d] [%d]/[%d] %s",
				utils.GetGPid(), id, count[k], j, len(links),
				l.AsString()))
			Objects.Links = append(Objects.Links, l)
			j++
		}

		// Specific loopback borders routes could
		// be generated from links (for each link
		// such route should be created)
		loopbacks := i.g.Opts.Network.Tunnels.TunnelsRoutes
		if loopbacks.Enabled {
			for _, l := range links {

				var r NetworkRoute
				r.Afi = ROUTE_IP6
				r.Prefix = l.Remote
				r.RouteType = ROUTETYPE_TUNNELS_LOOPBACK_STATIC

				r.Gateway = loopbacks.GatewayIp6Value
				r.Device = loopbacks.DeviceValue

				r.Mtu = loopbacks.MtuIp6
				r.Advmss = loopbacks.AdvmssIp6

				Objects.Routes = append(Objects.Routes, r)
			}
		}

		for k, p := range prefixes {

			// In tunnel mode we could have ecmp ipv4/ipv6 routes
			// it depends on count nexthops
			h := nexthops[k]
			rt := ROUTETYPE_TUNNELS_MPLS
			if len(h) > 1 {
				rt = ROUTETYPE_TUNNELS_ECMP_MPLS
			}

			var r NetworkRoute
			linkid := p.Community.Linkid

			// Simple route default with in one hop case
			r.Afi = p.RouteAfi()

			// We have some route table id multiplier
			r.Table = linkid * rtm

			r.Prefix = ROUTE_DEFAULT
			r.RouteType = rt
			r.Gateway = ROUTE_NOSPECIFICGATEWAY

			// Path has an array of labels, we need somehow
			// check if a number of labels more that 1, and
			// skip it? or what?
			if len(p.Labels) != 1 {
				i.g.Log.Error(fmt.Sprintf("%s %s error, path:'%s' has incorrect labels array count:'%d', skip it",
					utils.GetGPid(), id, p.AsMinimalString(), len(p.Labels)))
				continue
			}

			switch rt {
			case ROUTETYPE_TUNNELS_MPLS:
				r.Label = p.Labels[0]
				r.Device = fmt.Sprintf("%s-%s", prefix, p.HashNexthop())

				i.setMtu(&r)
				Objects.Routes = append(Objects.Routes, r)

			case ROUTETYPE_TUNNELS_ECMP_MPLS:
				i.setMtu(&r)

				if r.Afi == ROUTE_IP6 {
					// Very strange situation: we have ecmp mpls
					// routing request for IP6, but linux does not
					// support ecmp ipv6 via devices, so we need
					// fallback from ECMP routes to NORMAL routes
					// with some device selection policy, e.g.
					// hashed (linkid+p.HashNexthop) as first sorted

					fqdn := i.g.Opts.Runtime.Hostname
					nexthop := i.nexthopHashSelect(h, THashData{linkid: linkid, fqdn: fqdn})

					device := nexthop.Device
					r.Device = fmt.Sprintf("%s-%s", prefix, device)
					r.Label = nexthop.Label

					r.RouteType = ROUTETYPE_TUNNELS_MPLS
					Objects.Routes = append(Objects.Routes, r)
				}

				if r.Afi == ROUTE_IP4 {

					for _, enh := range h {

						var route NetworkEcmpRoute
						route.Label = enh.Label
						route.Device = fmt.Sprintf("%s-%s", prefix, enh.Device)
						route.Weight = enh.Weight
						route.Table = r.Table

						r.EcmpRoutes = append(r.EcmpRoutes, route)
					}

					Objects.Routes = append(Objects.Routes, r)
				}
			}

			// bypass for a list of networks given
			// as network prefixes for ip4/ip6
			bypass := i.g.Opts.Network.Tunnels.Bypass

			if bypass != nil && bypass.Enabled {
				networks := i.g.Opts.Network.NetworksDefaultIp4
				r.Mtu = bypass.MtuIp4
				r.Advmss = bypass.AdvmssIp4
				r.Gateway = bypass.GatewayIp4Value

				if r.Afi == ROUTE_IP6 {
					networks = i.g.Opts.Network.NetworksDefaultIp6
					r.Mtu = bypass.MtuIp6
					r.Advmss = bypass.AdvmssIp6
					r.Gateway = bypass.GatewayIp6Value
				}

				r.Device = bypass.DeviceValue
				r.RouteType = ROUTETYPE_TUNNELS_DEFAULT

				if len(r.Gateway) == 0 {
					continue
				}
				for _, p := range networks {
					r.Prefix = p
					Objects.Routes = append(Objects.Routes, r)
				}
			}
		}

		for k, r := range Objects.Routes {
			i.g.Log.Info(fmt.Sprintf("%s %s route [%d]/[%d] %s",
				utils.GetGPid(), id, k, len(Objects.Routes),
				r.AsString()))
		}

	}
	if class == config.NETWORK_CLASS_TRANSITS {
		rtm = i.g.Opts.Network.Transits.RouteTableMultiplier

		// here we have gateways for ip4 and ip6
		overrides := i.getOverrides()
		i.g.Log.Debug(fmt.Sprintf("%s %s overrides:'%s'", utils.GetGPid(),
			id, overrides.AsString()))

		transits := i.g.Opts.Network.Transits

		// routes for transit class: (1) mpls enc (2) default
		// (3) networks-default (w/o label)
		for _, p := range prefixes {
			linkid := p.Community.Linkid

			var r NetworkRoute
			r.Afi = p.RouteAfi()
			r.Table = linkid * rtm

			r.Prefix = ROUTE_DEFAULT
			r.RouteType = ROUTETYPE_TRANSITS_MPLS

			// Path has an array of labels, we need somehow
			// check if a number of labels more that 1, and
			// skip it? or what?
			if len(p.Labels) != 1 {
				i.g.Log.Error(fmt.Sprintf("%s %s error, path:'%s' has incorrect labels array count:'%d', skip it",
					utils.GetGPid(), id, p.AsMinimalString(), len(p.Labels)))
				continue
			}
			r.Label = p.Labels[0]
			r.Metric = transits.MetricEncap

			r.Gateway = overrides.GatewayIp4
			r.Mtu = transits.MtuDefaultIp4
			r.Advmss = transits.AdvmssDefaultIp4

			if r.Afi == ROUTE_IP6 {
				r.Gateway = overrides.GatewayIp6
				r.Mtu = transits.MtuDefaultIp6
				r.Advmss = transits.AdvmssDefaultIp6
			}

			r.Device = ROUTE_NOSPECIFICDEVICE
			// r.Gateway could be set to auto, meaning that
			// we should detect automatically

			if r.Gateway == "auto" {
				var route *NetworkRoute
				if route, err = i.getDefaultRoute(ObjectsSource, r.Afi); err != nil {
					i.g.Log.Error(fmt.Sprintf("%s %s error detecting default route, err:'%s'",
						utils.GetGPid(), id, err))
					return Objects, err
				}

				r.Gateway = route.Gateway
			}

			i.g.Log.Debug(fmt.Sprintf("%s %s [%s] setting parameters device:'%s' gateway:'%s'",
				utils.GetGPid(), id, RouteAsString(r.Afi), r.Device,
				r.Gateway))

			r.Proto = transits.Proto
			Objects.Routes = append(Objects.Routes, r)

			// fallback default route
			r.Metric = transits.MetricDefault
			r.RouteType = ROUTETYPE_TRANSITS_DEFAULT
			r.Label = 0
			Objects.Routes = append(Objects.Routes, r)

			// bypass for a list of networks given
			// as network prefixes for ip4/ip6
			bypass := i.g.Opts.Network.Transits.Bypass

			if bypass != nil && bypass.Enabled {
				networks := i.g.Opts.Network.NetworksDefaultIp4
				r.Mtu = bypass.MtuIp4
				r.Advmss = bypass.AdvmssIp4
				r.Gateway = bypass.GatewayIp4Value

				if r.Afi == ROUTE_IP6 {
					networks = i.g.Opts.Network.NetworksDefaultIp6
					r.Mtu = bypass.MtuIp6
					r.Advmss = bypass.AdvmssIp6
					r.Gateway = bypass.GatewayIp6Value
				}

				for _, p := range networks {
					r.Prefix = p
					Objects.Routes = append(Objects.Routes, r)
				}
			}
		}

		for k, r := range Objects.Routes {
			i.g.Log.Info(fmt.Sprintf("%s %s route [%d]/[%d] %s",
				utils.GetGPid(), id, k, len(Objects.Routes),
				r.AsString()))
		}
	}

	// Generating rule objects from generated before
	// routes objects
	for _, p := range prefixes {
		linkid := p.Community.Linkid

		var r NetworkRule
		r.Afi = p.RouteAfi()
		r.Table = linkid * rtm
		r.Mark = linkid

		Objects.Rules = append(Objects.Rules, r)
	}

	for k, r := range Objects.Rules {
		i.g.Log.Info(fmt.Sprintf("%s %s rule [%d]/[%d] %s",
			utils.GetGPid(), id, k, len(Objects.Rules),
			r.AsString()))
	}

	return Objects, err
}

func (*Ip) GetObjectRuleSlice(rules []NetworkRule) []Object {
	var out []Object
	for _, r := range rules {
		var o Object
		o = r
		out = append(out, o)
	}
	return out
}

func (*Ip) GetObjectRoutesSlice(routes []NetworkRoute) []Object {
	var out []Object
	for _, r := range routes {
		var o Object
		o = r
		out = append(out, o)
	}
	return out
}
func (*Ip) GetObjectLinksSlice(links []NetworkLink) []Object {
	var out []Object
	for _, r := range links {
		var o Object
		o = r
		out = append(out, o)
	}
	return out
}

type Object interface {
	// Key includes all object properties
	Key() string

	AsString() string

	// Specific method on create
	// and remove
	Create(g *config.CmdGlobal, ObjectsSource *TObjectsSource) error
	Remove(g *config.CmdGlobal, ObjectsSource *TObjectsSource) error
}

const (
	ACTION_CREATE  = 100
	ACTION_REMOVE  = 101
	ACTION_UNKNOWN = 0
)

func ActionAsString(action int) string {
	actions := map[int]string{
		ACTION_CREATE:  "CREATE",
		ACTION_REMOVE:  "REMOVE",
		ACTION_UNKNOWN: "UNKNOWN",
	}

	if _, ok := actions[action]; !ok {
		return actions[ACTION_UNKNOWN]
	}
	return actions[action]
}

type Action struct {
	ActionType int `json:"action-type"`

	// Object itself
	Object Object

	// Object source including placement
	ObjectsSource *TObjectsSource

	// Auxiliary field for debug
	Class string `json:"class"`
}

func (a *Action) Apply(g *config.CmdGlobal) error {
	var err error

	id := "(action)"
	if a.ActionType == ACTION_CREATE {
		if err = a.Object.Create(g, a.ObjectsSource); err != nil {
			g.Log.Error(fmt.Sprintf("%s %s error on action:'%s' object:'%s', err:'%s'",
				utils.GetGPid(), id, ActionAsString(a.ActionType),
				a.Object.AsString(), err))
			return err
		}
	}

	if a.ActionType == ACTION_REMOVE {
		if err = a.Object.Remove(g, a.ObjectsSource); err != nil {
			g.Log.Error(fmt.Sprintf("%s %s error on action:'%s' object:'%s', err:'%s'",
				utils.GetGPid(), id, ActionAsString(a.ActionType),
				a.Object.AsString(), err))
			return err
		}
	}

	return err
}

type Actions struct {
	RemoveActions []Action
	CreateActions []Action
}

func (m *Actions) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("remove:'%d'", len(m.RemoveActions)))
	out = append(out, fmt.Sprintf("create:'%d'", len(m.CreateActions)))

	return fmt.Sprintf("%s", strings.Join(out, ", "))
}

func (m *Actions) DumpActions(g *config.CmdGlobal) {
	id := "[actions]"
	for i, a := range m.RemoveActions {
		g.Log.Info(fmt.Sprintf("%s %s [%s] [REMOVE] [%d]/[%d] object: %s",
			utils.GetGPid(), id, strings.ToUpper(a.Class),
			i, len(m.RemoveActions),
			a.Object.AsString()))
	}

	for i, a := range m.CreateActions {
		g.Log.Info(fmt.Sprintf("%s %s [%s] [CREATE] [%d]/[%d] object: %s",
			utils.GetGPid(), id, strings.ToUpper(a.Class),
			i, len(m.CreateActions),
			a.Object.AsString()))
	}
}

func (m *Actions) Apply(g *config.CmdGlobal) error {
	var err error
	id := "(apply)"

	g.Log.Debug(fmt.Sprintf("%s %s applying actions %s",
		utils.GetGPid(), id, m.AsString()))

	for _, a := range m.RemoveActions {
		if err = a.Apply(g); err != nil {
			g.Log.Error(fmt.Sprintf("%s %s error applying 'remove' action, err:'%s'",
				utils.GetGPid(), id, err))
			return err
		}
	}

	for _, a := range m.CreateActions {
		if err = a.Apply(g); err != nil {
			g.Log.Error(fmt.Sprintf("%s %s error applying 'create' action, err:'%s'",
				utils.GetGPid(), id, err))
			return err
		}
	}

	return err
}

// Detecting a difference between objects lists and
// returing actions. Assuming that we could operate
// only REMOVE/CREATE. UPDATE is REMOVE and than CREATE
// actions
func (i *Ip) detectObjectActions(src []Object, dst []Object,
	ObjectsSource *TObjectsSource, class string) (Actions, error) {

	var err error
	var Actions Actions

	id := "(actions)"
	// Creating a map for objects at
	// src and with uniq object key
	msrc := make(map[string]Object)
	for _, s := range src {
		msrc[s.Key()] = s
	}
	mdst := make(map[string]Object)
	for _, s := range dst {
		mdst[s.Key()] = s
	}

	// Detecting CREATE and REMOVE actions
	for k, s := range msrc {
		if _, ok := mdst[k]; !ok {
			var A Action
			A.ActionType = ACTION_CREATE
			A.Object = s
			A.Class = class
			A.ObjectsSource = ObjectsSource
			Actions.CreateActions =
				append(Actions.CreateActions, A)
		}
	}

	for k, s := range mdst {
		if _, ok := msrc[k]; !ok {
			var A Action
			A.ActionType = ACTION_REMOVE
			A.Object = s
			A.Class = class
			A.ObjectsSource = ObjectsSource
			Actions.RemoveActions =
				append(Actions.RemoveActions, A)
		}
	}

	Actions.DumpActions(i.g)

	i.g.Log.Debug(fmt.Sprintf("%s %s [ACTIONS] [%s] remove:'%d', create:'%d'",
		utils.GetGPid(), id, class, len(Actions.RemoveActions),
		len(Actions.CreateActions)))

	return Actions, err
}
