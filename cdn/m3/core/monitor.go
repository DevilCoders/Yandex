package core

// Module contains a monitoring functions and processes
// and have methods to integrate with Monitor module

import (
	"fmt"
	"sort"
	"strings"
	"time"

	"a.yandex-team.ru/cdn/m3/cmd/config"
	"a.yandex-team.ru/cdn/m3/monitor"
	"a.yandex-team.ru/cdn/m3/utils"
)

// Integrating core monitoring with monitoring
func (c *Core) CoreMonitor(m *monitor.Monitor) {

	m.AddConfig(monitor.CheckConfig{ID: "m3-mpls-core", F: c.CoreMonitorMetrics})

	m.AddConfig(monitor.CheckConfig{ID: "m3-mpls-bgp", F: c.CoreMonitorMplsBgp})
	m.AddConfig(monitor.CheckConfig{ID: "m3-mpls-http", F: c.CoreMonitorMplsHttp})

	// Monitoring network objects
	m.AddConfig(monitor.CheckConfig{ID: "m3-mpls-objects", F: c.CoreMonitorMplsObjects})
}

func (c *Core) CoreMonitorMetrics() {

	// Need to know if metrics run as client or
	// as a server
	id := "m3-mpls-core"
	c.g.Log.Debug(fmt.Sprintf("%s core, id:'%s' checking monitoring mode:'%s'",
		utils.GetGPid(), id, c.g.Monitor.ModeAsString()))

	// detecting if bypass is set
	class := c.g.Opts.Runtime.NetworkClass
	bypassed := false
	if class == config.NETWORK_CLASS_TRANSITS {
		bypass := c.g.Opts.Network.Transits.Bypass
		if bypass != nil && bypass.Enabled {
			bypassed = true
		}
	}

	if class == config.NETWORK_CLASS_TUNNELS {
		bypass := c.g.Opts.Network.Tunnels.Bypass
		if bypass != nil && bypass.Enabled {
			bypassed = true
		}
	}

	check := monitor.Check{
		ID: id, Class: "m3",
		Timestamp: time.Now(), TTL: 120,
		Code: monitor.JUGGLER_OK,
		Message: fmt.Sprintf("%s network-class:'%s' mode:'%s' %s bypass:'%t'",
			c.g.Opts.Runtime.ProgramName,
			config.NetworkClassAsString(class),
			config.NetworkModeAsString(c.g.Opts.Runtime.NetworkMode),
			c.g.Opts.Runtime.AsString(), bypassed),
		Metrics: nil,
	}

	c.CoreCheckPush(&check, check.Code, check.Message, id,
		[]string{"m3", "mpls", config.NetworkClassAsString(class)})
}

func (c *Core) CoreCheckPush(check *monitor.Check, code int64,
	msg string, jid string, tags []string) {

	id := "(push)"

	check.Message = msg
	c.g.Log.Debug(fmt.Sprintf("%s core monitor id:'%s', msg:'%s'",
		utils.GetGPid(), check.ID, check.Message))
	check.Code = code

	if c.g.Opts.Juggler.Enabled {

		// Pushing juggler raw events
		juggler := monitor.TItemJuggler{
			Id:     check.Message,
			Status: check.Code,
		}

		var err error
		var rje monitor.TRawJugglerEnvelope
		c.g.Monitor.PushEvent(&rje, jid, juggler.Id,
			int(juggler.Status), tags)
		if err = c.g.Monitor.SendRawJuggler(rje); err != nil {
			c.g.Log.Error(fmt.Sprintf("%s error sending raw juggler events, err:'%s'",
				utils.GetGPid(), id, err))
		}
	}

	c.g.Monitor.PushCheck(*check)
}

func (c *Core) CoreMonitorMplsObjects() {

	id := "m3-mpls-objects"
	c.g.Log.Debug(fmt.Sprintf("%s monitor, id:'%s' checking monitoring mode:'%s'",
		utils.GetGPid(), id, c.g.Monitor.ModeAsString()))

	check := monitor.Check{
		ID: id, Class: "m3",
		Timestamp: time.Now(), TTL: 120,
		Code: monitor.JUGGLER_OK,
		Message: fmt.Sprintf("%s %s", c.g.Opts.Runtime.ProgramName,
			c.g.Opts.Runtime.AsString()),
		Metrics: nil,
	}

	// Getting network objects
	var objects TObjects
	var err error

	var StatusOptionsOverrides StatusOptionsOverrides
	StatusOptionsOverrides.Json = true
	StatusOptionsOverrides.Info = false

	if objects, err = c.networkStatus(&StatusOptionsOverrides); err != nil {
		check.Message = fmt.Sprintf("network status, err:'%s'", err)
		check.Code = monitor.JUGGLER_CRITICAL
		if strings.HasPrefix(fmt.Sprintf("%s", err), "404") {
			check.Code = monitor.JUGGLER_WARNING
		}

		c.CoreCheckPush(&check, check.Code, check.Message, id, []string{"m3", "mpls",
			config.NetworkClassAsString(c.g.Opts.Runtime.NetworkClass)})

		c.g.Log.Error(fmt.Sprintf("error status, err:'%s'", err))
		return
	}

	c.g.Log.Debug(fmt.Sprintf("%s monitor %s network objects received %s",
		utils.GetGPid(), id, objects.AsString()))

	var links []string
	for i, l := range objects.Links {
		c.g.Log.Debug(fmt.Sprintf("%s %s LINKS [%d]/[%d] %s",
			utils.GetGPid(), id, i, len(objects.Links),
			l.AsString()))

		links = append(links, fmt.Sprintf("'%s -> %s'",
			l.Ifname, l.Remote))
	}

	var routes []string
	for i, r := range objects.Routes {
		// Need to parse only encap/mpls routes
		c.g.Log.Debug(fmt.Sprintf("%s %s ROUTES [%d]/[%d] %s",
			utils.GetGPid(), id, i, len(objects.Routes),
			r.AsString()))

		if r.RouteType == ROUTETYPE_TUNNELS_MPLS {
			routes = append(routes, fmt.Sprintf("'(%s):%d:%s -> %d'",
				RouteAsString(r.Afi), r.Label, r.Device, r.Table))
		}

		if r.RouteType == ROUTETYPE_TUNNELS_ECMP_MPLS {
			routes = append(routes, fmt.Sprintf("'(%s) %d <- ecmp:%s'",
				RouteAsString(r.Afi), r.Table,
				r.EcmpRoutesAsString()))
		}

		if r.RouteType == ROUTETYPE_TRANSITS_MPLS {
			routes = append(routes, fmt.Sprintf("'(%s):%d -> %d'",
				RouteAsString(r.Afi), r.Label, r.Table))
		}
	}
	var rules []string
	for i, r := range objects.Rules {
		c.g.Log.Debug(fmt.Sprintf("%s %s RULES [%d]/[%d] %s",
			utils.GetGPid(), id, i, len(objects.Rules),
			r.AsString()))

		rules = append(rules, fmt.Sprintf("'(%s):0x%0x -> %d'",
			RouteAsString(r.Afi), r.Mark, r.Table))
	}

	class := c.g.Opts.Runtime.NetworkClass

	if class == config.NETWORK_CLASS_TUNNELS {
		if len(links) == 0 || len(routes) == 0 || len(rules) == 0 {
			check.Code = monitor.JUGGLER_CRITICAL
		}
	}

	if class == config.NETWORK_CLASS_TRANSITS {
		if len(routes) == 0 || len(rules) == 0 {
			check.Code = monitor.JUGGLER_CRITICAL
		}
	}

	metrics := make(monitor.Metrics)
	w := monitor.Value{Value: float64(len(links)), Id: "network-links"}
	//w.Tags = make(map[string]string)
	//w.Tags["location"]=
	metrics[w.Id] = w

	w = monitor.Value{Value: float64(len(routes)), Id: "network-routes"}
	metrics[w.Id] = w

	w = monitor.Value{Value: float64(len(rules)), Id: "network-rules"}
	metrics[w.Id] = w

	check.Metrics = &metrics

	if class == config.NETWORK_CLASS_TUNNELS {
		check.Message = fmt.Sprintf("%s [%d]:[%d]:[%d] links:[%d]:[%s] routes:[%d]:[%s] rules:[%d]:[%s]",
			c.g.Opts.Runtime.ProgramName, len(links), len(routes), len(rules),
			len(links), strings.Join(links, ", "), len(routes), strings.Join(routes, ", "),
			len(rules), strings.Join(rules, ", "))
	}

	if class == config.NETWORK_CLASS_TRANSITS {
		check.Message = fmt.Sprintf("%s [%d]/[%d]:[%d] routes:[%d]:[%s] rules:[%d]:[%s]",
			c.g.Opts.Runtime.ProgramName, len(routes), len(objects.Routes), len(rules),
			len(routes), strings.Join(routes, ", "),
			len(rules), strings.Join(rules, ", "))
	}

	c.CoreCheckPush(&check, check.Code, check.Message, id, []string{"m3", "mpls",
		config.NetworkClassAsString(c.g.Opts.Runtime.NetworkClass)})
}

// Monitoing http sources, it should be accessed, data
// retrieved and parsed as json, some information about
// links returned

func (c *Core) CoreMonitorMplsHttp() {
	var err error

	id := "m3-mpls-http"
	c.g.Log.Debug(fmt.Sprintf("%s monitor, id:'%s' checking monitoring mode:'%s'",
		utils.GetGPid(), id, c.g.Monitor.ModeAsString()))

	check := monitor.Check{
		ID: id, Class: "m3",
		Timestamp: time.Now(), TTL: 120,
		Code: monitor.JUGGLER_OK,
		Message: fmt.Sprintf("%s %s", c.g.Opts.Runtime.ProgramName,
			c.g.Opts.Runtime.AsString()),
		Metrics: nil,
	}

	url := c.g.Opts.M3.Source

	if strings.HasPrefix(url, "bgp") || len(url) == 0 {
		check.Message = fmt.Sprintf("http source is disabled, using bgp")
		check.Code = monitor.JUGGLER_WARNING

		c.CoreCheckPush(&check, check.Code, check.Message, id, []string{"m3", "mpls",
			config.NetworkClassAsString(c.g.Opts.Runtime.NetworkClass)})
		return
	}

	var paths []Path

	if paths, err = c.httpGetPaths(); err != nil {
		check.Message = fmt.Sprintf("error gettng links and prefixes, err:'%s'", err)
		check.Code = monitor.JUGGLER_CRITICAL

		c.CoreCheckPush(&check, check.Code, check.Message, id, []string{"m3", "mpls",
			config.NetworkClassAsString(c.g.Opts.Runtime.NetworkClass)})

		c.g.Log.Error(fmt.Sprintf("error status, err:'%s'", err))
		return

	}

	c.g.Log.Debug(fmt.Sprintf("%s monitor, id:'%s' got paths count:'%d'",
		utils.GetGPid(), id, len(paths)))

	metrics := make(monitor.Metrics)

	Neighbors := 1

	links := make(map[int]int)
	lo4 := make(map[string]int)
	lo6 := make(map[string]int)

	for _, p := range paths {

		label := 0
		if len(p.Labels) > 0 {
			label = int(p.Labels[0])
		}
		links[label] = links[label] + 1

		nexthop := ""
		if len(p.NextHops) > 0 {
			nexthop = p.NextHops[0]
		}

		lo6[nexthop] = lo6[nexthop] + 1
	}
	Links := len(links)
	Prefixes := len(paths)

	var lout []string
	var lprefixes4 []string
	var lprefixes6 []string

	for l, _ := range links {
		lout = append(lout, fmt.Sprintf("%d", l))
	}
	for l, _ := range lo4 {
		lprefixes4 = append(lprefixes4, l)
	}
	for l, _ := range lo6 {
		lprefixes6 = append(lprefixes6, l)
	}
	sort.Strings(lout)
	sort.Strings(lprefixes4)
	sort.Strings(lprefixes6)

	max := 10

	if len(lout) < max {
		max = len(lout)
	}
	LinksString := fmt.Sprintf("links:['%s'...]",
		strings.Join(lout[:max], ","))

	max = 10
	if len(lprefixes6) < max {
		max = len(lprefixes6)
	}
	Lo6String := fmt.Sprintf("lo6:['%s'...]",
		strings.Join(lprefixes6[:max], ","))

	w := monitor.Value{Value: float64(Neighbors), Id: "session-neighbors"}
	//w.Tags = make(map[string]string)
	//w.Tags["location"]=
	metrics[w.Id] = w

	w = monitor.Value{Value: float64(len(lprefixes4) + len(lprefixes6)), Id: "session-prefixes"}
	metrics[w.Id] = w

	w = monitor.Value{Value: float64(Links), Id: "session-links"}
	metrics[w.Id] = w

	check.Metrics = &metrics

	age := float64(time.Since(c.g.Opts.Runtime.T0).Seconds())

	check.Message = fmt.Sprintf("%s [%d]:[%d]:[%d] lo4:[%d] lo6:[%d] http sources:'%d' %s %s age:'%2.2f'",
		c.g.Opts.Runtime.ProgramName, Neighbors, Prefixes, Links, len(lo4), len(lo6), Neighbors, LinksString,
		Lo6String,
		age)

	c.CoreCheckPush(&check, check.Code, check.Message, id, []string{"m3", "mpls",
		config.NetworkClassAsString(c.g.Opts.Runtime.NetworkClass)})
}

// Monitoring bgp session: neighbours, prefixes, links
// their should match configured monitoring thresholds
func (c *Core) CoreMonitorMplsBgp() {

	id := "m3-mpls-bgp"
	c.g.Log.Debug(fmt.Sprintf("%s monitor, id:'%s' checking monitoring mode:'%s'",
		utils.GetGPid(), id, c.g.Monitor.ModeAsString()))

	check := monitor.Check{
		ID: id, Class: "m3",
		Timestamp: time.Now(), TTL: 120,
		Code: monitor.JUGGLER_OK,
		Message: fmt.Sprintf("%s %s", c.g.Opts.Runtime.ProgramName,
			c.g.Opts.Runtime.AsString()),
		Metrics: nil,
	}

	url := c.g.Opts.M3.Source

	if strings.HasPrefix(url, "http") {
		check.Message = fmt.Sprintf("bgp source is disabled, using http")
		check.Code = monitor.JUGGLER_WARNING

		c.CoreCheckPush(&check, check.Code, check.Message, id, []string{"m3", "mpls",
			config.NetworkClassAsString(c.g.Opts.Runtime.NetworkClass)})
		return
	}

	// Getting state information
	var paths []Path
	var err error

	var StatusOptionsOverrides StatusOptionsOverrides
	StatusOptionsOverrides.Json = true
	StatusOptionsOverrides.Info = false

	if paths, err = c.bgpStatus(&StatusOptionsOverrides); err != nil {
		check.Message = fmt.Sprintf("bgp status, err:'%s'", err)
		check.Code = monitor.JUGGLER_CRITICAL

		c.CoreCheckPush(&check, check.Code, check.Message, id, []string{"m3", "mpls",
			config.NetworkClassAsString(c.g.Opts.Runtime.NetworkClass)})

		c.g.Log.Error(fmt.Sprintf("error status, err:'%s'", err))
		return
	}

	c.g.Log.Debug(fmt.Sprintf("%s monitor %s paths received count:'%d'",
		utils.GetGPid(), id, len(paths)))

	// Got paths and configured monitored plan
	var m config.MonitoringOverride
	m.SessionNeighbors = c.g.Opts.Monitoring.SessionNeighbors
	m.SessionPrefixes = c.g.Opts.Monitoring.SessionPrefixes
	m.SessionLinks = c.g.Opts.Monitoring.SessionLinks

	loc := config.LocationAsString(c.g.Opts.Runtime.Location)

	if w, ok := c.g.Opts.Monitoring.Overrides[loc]; ok {
		m = w
	}
	m.SessionWaittime = c.g.Opts.Monitoring.SessionWaittime

	c.g.Log.Debug(fmt.Sprintf("%s monitor %s monitoring config '%s'",
		utils.GetGPid(), id, m.AsString()))

	// Calculating metrics and compare with monitoring configuration
	prefixes := make(map[string][]string)
	links := make(map[string][]string)
	neighbors := make(map[string]string)
	for _, p := range paths {
		n := p.Neighbor
		neighbors[n] = n

		prefix := fmt.Sprintf("%s/%d", p.Prefix, p.PrefixLen)
		prefixes[n] = append(prefixes[n], prefix)

		var labels []string
		for _, l := range p.Labels {
			labels = append(labels, fmt.Sprintf("%d", l))
		}
		sort.Strings(labels)

		nexthops := fmt.Sprintf("['%s']",
			strings.Join(p.NextHops, ", "))

		link := fmt.Sprintf("['%s':%d->%s]:%s",
			prefix, p.Community.Linkid,
			labels, nexthops)

		links[n] = append(links[n], link)
	}

	c.g.Log.Debug(fmt.Sprintf("%s monitor %s neighbors:'%s'",
		utils.GetGPid(), id, MapAsString(neighbors)))

	age := float64(time.Since(c.g.Opts.Runtime.T0).Seconds())

	Neighbors := uint32(len(neighbors))
	Prefixes, PrefixesString := MapStringAsString(prefixes)
	Links, LinksString := MapStringAsString(links)

	// listing links for debug
	count := 0
	for n, ll := range links {
		for _, l := range ll {
			c.g.Log.Debug(fmt.Sprintf("%s %s LINKS [%d]/[%d] n:'%s' ->  %s",
				utils.GetGPid(), id, count, Links, n, l))
			count++
		}
	}

	if Neighbors != m.SessionNeighbors || Prefixes != m.SessionPrefixes ||
		Links != m.SessionLinks {
		check.Code = monitor.JUGGLER_CRITICAL
	}

	if check.Code == monitor.JUGGLER_CRITICAL &&
		age < float64(m.SessionWaittime) {
		check.Code = monitor.JUGGLER_WARNING
	}

	metrics := make(monitor.Metrics)
	w := monitor.Value{Value: float64(Neighbors), Id: "session-neighbors"}
	//w.Tags = make(map[string]string)
	//w.Tags["location"]=
	metrics[w.Id] = w

	w = monitor.Value{Value: float64(Prefixes), Id: "session-prefixes"}
	metrics[w.Id] = w

	w = monitor.Value{Value: float64(Links), Id: "session-links"}
	metrics[w.Id] = w

	check.Metrics = &metrics

	check.Message = fmt.Sprintf("%s [%d]:[%d]:[%d] neighbors:'%s' prefixes:'%s' links:'%s' age:'%2.2f'",
		c.g.Opts.Runtime.ProgramName, Neighbors, Prefixes, Links,
		MapAsString(neighbors), PrefixesString,
		LinksString, age)

	c.CoreCheckPush(&check, check.Code, check.Message, id, []string{"m3", "mpls",
		config.NetworkClassAsString(c.g.Opts.Runtime.NetworkClass)})
}

func MapAsString(m map[string]string) string {
	var out []string
	for _, v := range m {
		out = append(out, v)
	}
	return fmt.Sprintf("[%d]:[%s]", len(out),
		strings.Join(out, ", "))
}

func MapStringAsString(m map[string][]string) (uint32, string) {
	var out []string
	count := 0
	for k, v := range m {
		count += len(v)
		w := fmt.Sprintf("[%s]:[%d]:['%s']",
			k, len(v), strings.Join(v, ", "))
		out = append(out, w)
	}

	return uint32(count), fmt.Sprintf("[%d]:%s", len(out),
		strings.Join(out, ", "))
}
