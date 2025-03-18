package config

import (
	"bytes"
	"context"
	"crypto/tls"
	"crypto/x509"
	"errors"
	"fmt"
	"io/ioutil"
	"log/syslog"
	"net"
	"net/http"
	"os"
	"strings"
	"time"

	"github.com/sirupsen/logrus"
	"github.com/spf13/cobra"
	"github.com/spf13/viper"

	"a.yandex-team.ru/cdn/m3/events"
	"a.yandex-team.ru/cdn/m3/monitor"
	"a.yandex-team.ru/cdn/m3/utils"
)

const (
	PROGRAM_NAME = "m3"

	UintSize = 32 << (^uint(0) >> 32 & 1)
	MaxInt   = 1<<(UintSize-1) - 1
)

var defaultConf = []byte(`
#
# m3 configuration
#

log:
  # logging format could be "string" or "json"
  format: "string"

  # possible values: "stdout" - output to console (actually to
  # syslog if systemd based setup, "syslog" - via native syslog,
  # please be sure to have rsyslog.d configuration, and the
  # last "file" as "/var/log/m3/m3.log"
  log: "stdout"

  # level of debugging could be "debug", "info", could be
  # overrided by command line switch
  level: "debug"

bgp:
  # m3 daemon establishes bgp session with a list
  # of peer defined below, there are also some
  # specific settings to control bgp processes

  # local and peer as, local router-id
  as: 13238
  peer-as: 13238
  router-id: "5.45.193.133"

  # default peer safi (if not present in peer
  # definition in peers array)
  default-safi: "MPLS_LABEL"

  # a list of peers, assuming that remote port
  # is the same for all of them
  #peers: [ "87.250.233.160", "141.8.136.223" ]
  #peers: [ "MPLS_LABEL@2a02:6b8:0:202b::252" ]

  # std-vpnerr-eth0-803.yndx.net, see [1]
  # [1] https://st.yandex-team.ru/TRAFFIC-11453
  peers: [ "MPLS_VPN@2a02:6b8:0:370f::77" ]

  # remote and local ports for bgp sessions
  remote-port: 179

  # local-address could be empty
  #local-address: "2a02:6b8:b010:a4fc::120"

  # local-port, bgp itself starts, could
  #  be set to -1?
  local-port: -1

  # passive or active session, for mpls
  # transit we have here "false"
  passive: false

  # m3 process start a number of routines, one
  # is bgp session handling if enabled, if not
  # it could be started from command line
  enabled: true

  # bgp produces a lot of bgp messages, by default
  # bgp messages dump logging is disabled (they
  # are logged as json parsed gobgp objects)
  messages-dumped: false

  # overrides for specific location in
  # multi-configuraton models
  overrides:
     "kiv":
        peers: [ "87.250.233.160", "141.8.136.223" ]
     "rad":
        peers: [ "87.250.233.160", "141.8.136.223" ]

network:
  # options to set network configurations, e.g. mode
  # container/host. "container" means that m3 runs on
  # dom0 host and controls containers inside dom0
  mode: "host"

  # two possible classes to manage: "transits" and
  # "tunnels"
  #
  # (a) "transits" manages rules and tables with some
  # fallback defaults in tables (if mpls lables and
  # routes are deleted from configuration)

  # (b) "tunnels" establishes rules, tables and links
  # as tunnles from local address to next-hops. m3
  # has internal auto-configuration and for regions
  # dom0 nodes **forcefully** set class to "transits"
  class: "tunnels"

  # options for container mode control if mode
  # is set to "contaner" m3 filters a type of
  # container to control
  container:
    # type of containers to control over m3
    types: [ "proxy", "strm" ]

  # startup time in seconds to wait for
  # bgp session establish state. if startup
  # time is set to "0" it could lead to
  # network objects recreation/flapping as
  # m3 is restarting
  startup-waittime: 30

  # as far as all traffic labelled first as fwmark
  # and then as mpls lables are routed via the same
  # table, we need, in transit (tunnels?) have
  # routes not to label them as mpls and route
  # prefixes below as default on host
  networks-default-ip6:
    - "2a02:6b8::/29"
    - "2a02:5180::/32"
    - "2620:10f:d000::/44"

  networks-default-ip4:
    - "185.32.184.0/23"
    - "141.8.128.0/18"
    - "5.45.192.0/18"
    - "185.71.76.0/22"
    - "178.154.128.0/19"
    - "5.255.192.0/18"
    - "77.75.152.0/21"
    - "77.88.0.0/18"
    - "93.158.128.0/18"
    - "199.21.96.0/22"
    - "95.108.128.0/17"
    - "109.235.160.0/21"
    - "87.250.224.0/19"
    - "37.9.64.0/18"
    - "37.140.128.0/18"
    - "213.180.192.0/19"
    - "199.36.240.0/22"
    - "100.43.64.0/19"
    - "93.158.160.0/20"
    - "178.154.160.0/19"
    - "178.154.192.0/19"

  # options for "tunnels" class configuration
  tunnels:
    # ifname/device prefix for tunnels links
    # object devices
    device-prefix: "strm"

    # table proto filtering for "routes", in
    # tunnels mode historically, no any specific
    # proto was established
    proto: "all"

    # local-address fot tunnels, is it possible
    # to detect it automatically?
    local-address: "2a02:6b8:b010:a4fc::120"
    #local-address: "2a02:6b8:c20:1ce:0:584:d6c0:0"

    # encap-dport sets a remote encap
    # destination port
    encap-dport: 6635

    # route table multiplication factor
    route-table-multiplier: 1000

    # ip6 ecmp algorithm selection: in ip6 ecmp
    # we do not have linux routing mech in case
    # of dev (used for tunnels). so we need some
    # algorithm selection. hash:"linkid - hashed linkid
    # hash:linkid+host - hashed linkid+hostname of
    # instance, hash:linkid+host is used by default
    ecmp-ip6-algo: "hash:linkid+host"

    # specific parameters to override
    # numtxqueues, numrxqueues
    # txqueuelen
    numtxqueues: 1
    numrxqueues: 1
    txqueuelen: 1000

    # some specific route properties,
    # if they set - they used in
    # route creation [1]
    # https://st.yandex-team.ru/TRAFFIC-11756
    mpls-routes-mtu:

      # if not enabled routes in mpls
      # tables created without any mtu
      # advmss set
      enabled: true

      mtu-default-ip6: 1450
      mtu-default-ip4: 1450

      advmss-default-ip6: 1390
      advmss-default-ip4: 1410

    # for tunnels we need static
    # specific for each border
    # loopback address, border
    # tunnels peers routes
    tunnels-routes:
       # if set to true specific
       # routes should be created
       enabled: true

       # for these settings see bypass
       # section below
       device: "auto"
       gateway-ip6: "auto"

       advmss-ip6: 8900
       mtu-ip6: 8840

    # bypass for networks-default-ip6 and
    # networks-default-ip4: for networks
    # from *-default-ip* we need to skip
    # mpls tunnels/transits routing. It is
    # done via specific routes in tables.
    bypass:

      # to enable bypass network route
      # set "enabled" in bypase section to true
      enabled: true

      # in rtc there is a specific device
      # treated as bridge to route in dom0.
      # set to empty string (default: "")
      # to skip device specific routes
      device: "veth"

      # gateway-ip6 and gateway-ip4 define
      # if bypass should be created or not
      # empty string as addresses treated as
      # no creation is done (default: "")
      gateway-ip6: "2a02:6b8:c20:21b::badc:ab1e"

      # mtu and advmss settings for
      # bypass routes ip6 and ip4
      mtu-ip6: 8910

      # if set zero, not used in rules
      # creation (default: 0)
      advmss-ip6: 0

  # options from "transits" class
  transits:

    # transits have more sophisticated method
    # it established tables with predefined, see
    # [1] /etc/iproute2/rt_protos
    proto: "xorp"

    # route table multiplication factor
    route-table-multiplier: 1

    # transits should generate a route table
    # with specfied gateway in destination
    # network for ip4 and ip6

    gateway-ip4: "37.9.93.158"
    gateway-ip6: "2a02:6b8:0:70c::1"

    # encap mpls routes should have a metric of '100'
    # while all the rest in specific table '1000'
    metric-encap: 100
    metric-default: 1000

    # mtu and advmss for default routes
    mtu-default-ip6: 1450
    mtu-default-ip4: 1450

    advmss-default-ip6: 1390
    advmss-default-ip4: 1410

    # specific routes inside Yandex networks
    # should have jumbo mtu set on
    mtu-jumbo-ip6: 8900
    mtu-jumbo-ip4: 8900

    advmss-jumbo-ip6: 8840
    advmss-jumbo-ip4: 8860

    # specific overrides
    overrides:
      "kiv":
         gateway-ip4: "37.9.93.158"
         gateway-ip6: "2a02:6b8:0:70c::1"

      "rad":
         gateway-ip4: "37.9.93.190"
         gateway-ip6: "2a02:6b8:0:70d::1"

objects:
  # some objects in processing should be classified
  # and correctly parsed, some detection settings
  # below grouped for different types of objects

  links:
    # filtering links as min and max values
    # for linkid, e.g. links > 1000 - is not
    # peer links, sometimes it should be
    # skipped from processing

    match:
      # minimal and maximal linkid to
      # process, if not defined, m3 uses
      # default values of 0, MAXINT
      min: 0
      max: 1000

    # link is detected from "specical" community
    # number in a from high:N + low:M, N = 13238
    # M = 10000 + linkid

    high: 13238
    low: 10000

    # filtering links: in some installation we
    # need only a set of links allowed, e.g. kiv+rad
    # has specific links but bgp sends more

    filter:
      # filter is organizaed to match
      # "location" of node or its fqdn
      "rad":
         enabled: true
         links: [ 151, 159 ]
      "kiv":
         enabled: true
         links: [ 154, 155, 209 ]

      # disabling filtering on fqdn basis
      "kiv-srv05.regions.yandex.net":
         enabled: true
         links: [ 154, 155, 209 ]

monitoring:
  # monitoring for neighbours (established sessions)
  # prefixes counts links counts per location and
  # timeout for session to established

  # session-waittime number of seconds for bgp session
  # to established
  session-waittime: 60

  # a number of neighbors
  session-neighbors: 1

  # a number of prefixes per session e.g. for kiv and rad
  # it should be "3" and "2" transit links, see
  # also filter settings: "0.0.0.0/0" and "::0/0"
  session-prefixes: 4

  # a number of links per session, a link is an object
  # "154:171", "155:6156", defined as "linkid:mpls"
  session-links: 4

  # overrides monitoring settings could be
  # used for specific location (if defined)
  overrides:
    "kiv":
      # 87.250.233.160 and 141.8.136.223
      session-neighbors: 2

      # 0.0.0.0/0 and ::0/0
      session-prefixes: 12

      # ip4:154:171,155:6156,209:11481 + ip6:154:172,155:6155,209:11482
      session-links: 12

    "rad":
      # 87.250.233.160 and 141.8.136.223
      session-neighbors: 2

      # 0.0.0.0/0 and ::0/0
      session-prefixes: 8

      # ip4:154:171,155:6156,209:11481 + ip6:154:172,155:6155,209:11482
      session-links: 8

solomon:
  # pushing data into solomon installation
  # enabling solomon push metrics
  enabled: true

  # endpoint to push metrics into solomon
  # api/v2/push?project=%s&cluster=%s&service=%s
  endpoint: "http://solomon.yandex.net/api/v2"

  # project part of metrics
  project: "cdn"

  # cluster part of metrics
  cluster: "cdn-hx"

  # token for robot-dns
  token: ""

juggler:
  # juggler section for raw events
  # if enabled a list of raw events
  # are sent to endpoint with url below
  enabled: true

  # juggler client has a local address socket
  # for raw events
  url: "http://127.0.0.1:31579/events"

m3:
  # m3 socket to listen (for monitoring,
  # controlling and statistics via http rest api
  socket: "/var/run/m3.socket"

  # detach process mode: pidfile
  pidfile: "/var/run/m3.pid"

  # m3 could logrotate log file itself, it
  # could be scheduled to move log file
  # to old log filename and recreate new log
  logrotate: false

  # m3 could fetch data from bgp peers or
  # from http endpoint as defined in [1]
  # [1] TRAFFIC-12430. "bgp" is defined
  # in "bgp" section above http is defined
  # right here as http url
  #source: "bgp"
  source: "https://env-8e166069-1f3a-4106-86c8-eb3a8d6fff6b.n1.test.racktables.yandex-team.ru/export/peers-report.php?format=json"

lxd:
  # lxd api is used to manipulate lxd
  # containers network objects, executing
  # commands in the namespace of container

  # lxd socket
  socket: "/var/lib/lxd/unix.socket"

`)

type ConfigOverrides struct {
	Debug bool
	Log   string
}

func (o *ConfigOverrides) AsString() string {
	var out []string
	out = append(out, fmt.Sprintf("debug:'%t'", o.Debug))
	out = append(out, fmt.Sprintf("log:'%s'", o.Log))

	return fmt.Sprintf("%s", strings.Join(out, ","))
}

// logging could be (1) syslog via systemd
// (2) syslog directly (3) file (as last
// variant)

const (
	LOGTYPE_SYSLOG  = 0
	LOGTYPE_STDOUT  = 1
	LOGTYPE_FILE    = 2
	LOGTYPE_UNKNOWN = -1
)

func LogTypeAsString(logtype int) string {
	types := map[int]string{
		LOGTYPE_SYSLOG:  "log+syslog",
		LOGTYPE_STDOUT:  "log+stdout",
		LOGTYPE_FILE:    "log+file",
		LOGTYPE_UNKNOWN: "log+unknown",
	}

	if _, ok := types[logtype]; !ok {
		return types[LOGTYPE_UNKNOWN]
	}
	return types[logtype]
}

type TLogOptions struct {
	Format string
	Level  string
	Log    string
}

func (t *TLogOptions) AsString() string {
	var out []string
	out = append(out, fmt.Sprintf("level:'%s'", t.Level))
	out = append(out, fmt.Sprintf("log:'%s'", t.Log))
	return fmt.Sprintf("%s", strings.Join(out, ","))
}

type TLog struct {
	// LogType could be LOG_SYSLOG
	// LOG_STDOUT or LOG_FILE
	LogType int

	Log        *logrus.Logger
	LogOptions *TLogOptions

	File *os.File

	syslog *syslog.Writer
}

func (log *TLog) Debug(str string) {
	if log.LogType == LOGTYPE_STDOUT || log.LogType == LOGTYPE_FILE {
		if log.Log != nil {
			log.Log.Debug(str)
		}
	}
	if log.LogType == LOGTYPE_SYSLOG {
		log.syslog.Debug(fmt.Sprintf("level=%s %s", "debug", str))
	}
}

func (log *TLog) Info(str string) {
	if log.LogType == LOGTYPE_STDOUT || log.LogType == LOGTYPE_FILE {
		if log.Log != nil {
			log.Log.Info(str)
		}

	}
	if log.LogType == LOGTYPE_SYSLOG {
		log.syslog.Info(fmt.Sprintf("level=%s %s", "info", str))
	}
}

func (log *TLog) Error(str string) {
	if log.LogType == LOGTYPE_STDOUT || log.LogType == LOGTYPE_FILE {
		if log.Log != nil {
			log.Log.Error(str)
		}
	}
	if log.LogType == LOGTYPE_SYSLOG {
		log.syslog.Err(fmt.Sprintf("level=%s %s", "error", str))
	}
}

func (log *TLog) SetLogLevel(level string) error {
	if log.Log == nil {
		return errors.New("log not initialized")
	}
	l, err := logrus.ParseLevel(level)
	if err != nil {
		return err
	}
	log.Log.Level = l
	return nil
}

func (log *TLog) SetLogOut(out string) error {
	if log.Log == nil {
		return errors.New("log not initialized")
	}

	if log.LogType == LOGTYPE_STDOUT || log.LogType == LOGTYPE_FILE {
		switch out {
		case "stdout":
			log.Log.Out = os.Stdout
		case "stderr":
			log.Log.Out = os.Stderr
		default:
			f, err := os.OpenFile(log.LogOptions.Log,
				os.O_RDWR|os.O_CREATE|os.O_APPEND,
				0644)
			if err != nil {
				return err
			}
			log.Log.Out = f
			log.File = f

			logrus.SetOutput(log.File)
			logrus.SetLevel(logrus.DebugLevel)
		}
	}
	return nil
}

type LogrotateOptions struct {
	// if we need move file to old
	// named file or not, if not
	// we just create new file and use it
	Move bool `json:"move"`
}

func (l *LogrotateOptions) AsString() string {
	return fmt.Sprintf("move:'%t'", l.Move)
}

func (log *TLog) RotateLog(LogrotateOptions *LogrotateOptions) error {
	var err error

	id := "(rotate)"
	name := log.LogOptions.Log
	log.Debug(fmt.Sprintf("%s %s request to rotate log:'%s' logtype:'%s'",
		utils.GetGPid(), id, name, LogTypeAsString(log.LogType)))

	if LogrotateOptions != nil {
		log.Debug(fmt.Sprintf("%s %s logtate option: %s",
			utils.GetGPid(), id, LogrotateOptions.AsString()))
	}

	if log.Log == nil {
		return errors.New("log not initialized")
	}

	if log.LogType == LOGTYPE_FILE || name != "stdout" {
		f := log.File
		if f == nil {
			log.Error(fmt.Sprintf("%s %s error file description is not found",
				utils.GetGPid(), id))
			return errors.New("log not initialized")
		}
		if err := f.Close(); err != nil {
			log.Error(fmt.Sprintf("%s %s error closing file, err:'%s'",
				utils.GetGPid(), id, err))
			return err
		}

		if LogrotateOptions != nil && LogrotateOptions.Move {
			rotatename := fmt.Sprintf("%s.old", name)
			log.Debug(fmt.Sprintf("%s %s renaming file:'%s' -> '%s'",
				utils.GetGPid(), id, name, rotatename))

			err := os.Rename(name, rotatename)
			if err != nil {
				return err
			}
		}

		log.SetLogOut(log.LogOptions.Log)

		log.Debug(fmt.Sprintf("%s %s logfile:'%s' is rotated",
			utils.GetGPid(), id, name))
	}
	return err
}

func (log *TLog) UpdateLog() error {
	var err error
	if log.LogOptions == nil {
		return errors.New("log file not found")
	}
	if log.LogType == LOGTYPE_STDOUT || log.LogType == LOGTYPE_FILE {
		if err = log.SetLogLevel(log.LogOptions.Level); err != nil {
			return err
		}
		if err = log.SetLogOut(log.LogOptions.Log); err != nil {
			return err
		}
	}
	return err
}

func (log *TLog) UpdateOverrides(Overrides ConfigOverrides) error {
	if log.LogOptions == nil {
		return errors.New("log file not found")
	}
	if log.LogType == LOGTYPE_STDOUT || log.LogType == LOGTYPE_FILE {

		if Overrides.Debug {
			log.LogOptions.Level = "debug"
		}
		if len(Overrides.Log) > 0 {
			log.LogOptions.Log = Overrides.Log
		}
	}
	return log.UpdateLog()
}

func CreateLog(opts *ConfYaml) (*TLog, error) {

	var err error

	// LOG_STDOUT and LOG_FILE are implemented via logrus
	// LOG_SYSLOG is implemented via syslog
	var Log TLog
	Log.Log = logrus.New()

	LogOptions := opts.LogOptions

	// Need detect a Logtype: assuming that log
	// options (if defined) override config options

	Log.LogType = LOGTYPE_STDOUT
	if LogOptions.Log == "syslog" {
		Log.LogType = LOGTYPE_SYSLOG
		levels := syslog.LOG_WARNING | syslog.LOG_DAEMON | syslog.LOG_DEBUG
		if Log.syslog, err = syslog.Dial("", "", levels, PROGRAM_NAME); err != nil {
			return nil, errors.New(fmt.Sprintf("log error, err:'%s'", err))
		}
		return &Log, nil
	}

	if LogOptions.Log != "stdout" {
		Log.LogType = LOGTYPE_FILE
	}

	Log.LogOptions = LogOptions
	Log.Log.Formatter = &logrus.TextFormatter{
		TimestampFormat: "2006/01/02 - 15:04:05.000",
		FullTimestamp:   true,
	}

	if err = Log.UpdateLog(); err != nil {
		return nil, errors.New(fmt.Sprintf("log error, err:'%s'", err.Error()))
	}

	return &Log, nil
}

type CmdGlobal struct {
	Cmd  *cobra.Command
	Opts *ConfYaml
	Log  *TLog

	Events  *events.Events
	Monitor *monitor.Monitor

	// some singletons (e.g http to reuse pool of
	// connections)
	Transport        *http.Transport
	TransportSkipTls *http.Transport
	TransportUnix    *http.Transport
}

type Bypass struct {
	Enabled bool `yaml:"enabled"`

	// Configuration switches could be
	// real valus or "auto"
	Device     string `yaml:"device"`
	GatewayIp6 string `yaml:"gateway-ip6"`
	GatewayIp4 string `yaml:"gateway-ip4"`

	// Values calculated per container
	DeviceValue     string `yaml:"device-value"`
	GatewayIp6Value string `yaml:"gateway-ip6-value"`
	GatewayIp4Value string `yaml:"gateway-ip4-value"`

	// optional overrides for gateways
	// w.r.t location
	Overrides map[string]TransitsOverride `yaml:"overrides"`

	MtuIp6 uint32 `yaml:"mtu-ip6"`
	MtuIp4 uint32 `yaml:"mtu-ip4"`

	AdvmssIp6 uint32 `yaml:"advmss-ip6"`
	AdvmssIp4 uint32 `yaml:"advmss-ip4"`
}

func (b *Bypass) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("enabled:'%t'", b.Enabled))
	out = append(out, fmt.Sprintf("device:'%s'", b.Device))
	out = append(out, fmt.Sprintf("gateway-ip6:'%s'", b.GatewayIp6))
	out = append(out, fmt.Sprintf("gateway-ip4:'%s'", b.GatewayIp4))

	out = append(out, fmt.Sprintf("mtu-ip6:'%d'", b.MtuIp6))
	out = append(out, fmt.Sprintf("mtu-ip4:'%d'", b.MtuIp4))

	out = append(out, fmt.Sprintf("advmss-ip6:'%d'", b.AdvmssIp6))
	out = append(out, fmt.Sprintf("advmss-ip4:'%d'", b.AdvmssIp4))

	return fmt.Sprintf("%s", strings.Join(out, ","))
}

func ConfBypass(prefix string) *Bypass {
	var b Bypass

	b.Enabled = viper.GetBool(fmt.Sprintf("%s.enabled", prefix))
	b.Device = viper.GetString(fmt.Sprintf("%s.device", prefix))

	b.GatewayIp6 = viper.GetString(fmt.Sprintf("%s.gateway-ip6", prefix))
	b.GatewayIp4 = viper.GetString(fmt.Sprintf("%s.gateway-ip4", prefix))

	b.Overrides = make(map[string]TransitsOverride)
	{
		m := viper.GetStringMap(fmt.Sprintf("%s.overrides", prefix))
		for k, v := range m {
			var T TransitsOverride
			for t, w := range v.(map[string]interface{}) {
				switch t {
				case "gateway-ip4":
					T.GatewayIp4 = w.(string)
				case "gateway-ip6":
					T.GatewayIp6 = w.(string)
				}
			}
			b.Overrides[k] = T
		}
	}

	b.MtuIp6 = viper.GetUint32(fmt.Sprintf("%s.mtu-ip6", prefix))
	b.MtuIp4 = viper.GetUint32(fmt.Sprintf("%s.mtu-ip4", prefix))

	b.AdvmssIp6 = viper.GetUint32(fmt.Sprintf("%s.advmss-ip6", prefix))
	b.AdvmssIp4 = viper.GetUint32(fmt.Sprintf("%s.advmss-ip4", prefix))

	return &b
}

type BgpOverrides struct {
	Peers []string `yaml:"peers"`
}

const (
	DEFAULT_SAFI = "MPLS_LABEL"
)

type SectionBgp struct {
	As         uint32   `yaml:"as"`
	PeerAs     uint32   `yaml:"peer-as"`
	RouterId   string   `yaml:"router-id"`
	Peers      []string `yaml:"peers"`
	RemotePort int32    `yaml:"remote-port"`
	LocalPort  int32    `yaml:"local-port"`

	DefaultSafi string `yaml:"default-safi"`

	LocalAddress string `yaml:"local-address"`
	Passive      bool   `yaml:"passive"`
	Enabled      bool   `yaml:"enabled"`

	MessagesDumped bool `yaml:"messages-dumped"`

	Overrides map[string]BgpOverrides
}

func (s *SectionBgp) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("as:'%d'", s.As))
	out = append(out, fmt.Sprintf("peer-as:'%d'", s.PeerAs))
	out = append(out, fmt.Sprintf("router-id:'%s'", s.RouterId))
	out = append(out, fmt.Sprintf("peers:['%s']", strings.Join(s.Peers, ",")))
	out = append(out, fmt.Sprintf("remote-port:'%d'", s.RemotePort))
	out = append(out, fmt.Sprintf("default-safi:'%s'", s.DefaultSafi))

	if len(s.LocalAddress) > 0 {
		out = append(out, fmt.Sprintf("local-address:'%s'", s.LocalAddress))
	}

	out = append(out, fmt.Sprintf("local-port:'%d'", s.LocalPort))
	out = append(out, fmt.Sprintf("passive:'%t'", s.Passive))
	out = append(out, fmt.Sprintf("enabled'%t'", s.Enabled))
	out = append(out, fmt.Sprintf("messages-dumped'%t'", s.MessagesDumped))

	return fmt.Sprintf("%s", strings.Join(out, ", "))
}

type ConfYaml struct {
	Runtime    SectionRuntime
	M3         SectionM3      `yaml:"m3"`
	Bgp        SectionBgp     `yaml:"bgp"`
	Solomon    SectionSolomon `yaml:"solomon"`
	Juggler    SectionJuggler `yaml:"juggler"`
	Objects    SectionObjects `yaml:"objects"`
	LogOptions *TLogOptions

	Overrides *ConfigOverrides

	Monitoring SectionMonitoring `yaml:"monitoring"`
	Network    SectionNetwork    `yaml:"network"`

	Lxd SectionLxd `yaml:"lxd"`
}

type SectionLxd struct {
	Socket string `yaml:"socket"`
}

type MonitoringOverride struct {
	SessionWaittime  uint32 `yaml:"session-waittime"`
	SessionNeighbors uint32 `yaml:"session-neighbors"`
	SessionPrefixes  uint32 `yaml:"session-prefixes"`
	SessionLinks     uint32 `yaml:"session-links"`
}

func (m *MonitoringOverride) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("waittime:'%d'",
		m.SessionWaittime))
	out = append(out, fmt.Sprintf("neighbors:'%d'",
		m.SessionNeighbors))
	out = append(out, fmt.Sprintf("prefixes:'%d'",
		m.SessionPrefixes))
	out = append(out, fmt.Sprintf("links:'%d'",
		m.SessionLinks))

	return fmt.Sprintf("%s", strings.Join(out, "; "))
}

type SectionMonitoring struct {
	SessionWaittime  uint32 `yaml:"session-waittime"`
	SessionNeighbors uint32 `yaml:"session-neighbors"`
	SessionPrefixes  uint32 `yaml:"session-prefixes"`
	SessionLinks     uint32 `yaml:"session-links"`

	Overrides map[string]MonitoringOverride `yaml:"overrides"`
}

const (
	NETWORK_MODE_HOST      = 1001
	NETWORK_MODE_CONTAINER = 1002
	NETWORK_MODE_UNKNOWN   = 0

	NETWORK_CLASS_TUNNELS  = 2001
	NETWORK_CLASS_TRANSITS = 2002
	NETWORK_CLASS_UNKNOWN  = 0

	NETWORK_MODE_HOST_STR      = "host"
	NETWORK_MODE_CONTAINER_STR = "container"
	NETWORK_MODE_UNKNOWN_STR   = "unknown"

	NETWORK_CLASS_TUNNELS_STR  = "tunnels"
	NETWORK_CLASS_TRANSITS_STR = "transits"
	NETWORK_CLASS_UNKNOWN_STR  = "unknown"
)

func NetworkModeAsString(mode int) string {
	modes := map[int]string{
		NETWORK_MODE_HOST:      NETWORK_MODE_HOST_STR,
		NETWORK_MODE_CONTAINER: NETWORK_MODE_CONTAINER_STR,
		NETWORK_MODE_UNKNOWN:   NETWORK_MODE_UNKNOWN_STR,
	}

	if _, ok := modes[mode]; !ok {
		return modes[NETWORK_MODE_UNKNOWN]
	}
	return modes[mode]
}

func NetworkClassAsString(class int) string {
	classes := map[int]string{
		NETWORK_CLASS_TUNNELS:  NETWORK_CLASS_TUNNELS_STR,
		NETWORK_CLASS_TRANSITS: NETWORK_CLASS_TRANSITS_STR,
		NETWORK_CLASS_UNKNOWN:  NETWORK_CLASS_UNKNOWN_STR,
	}

	if _, ok := classes[class]; !ok {
		return classes[NETWORK_CLASS_UNKNOWN]
	}
	return classes[class]
}

type SectionNetwork struct {
	Mode  string `yaml:"mode"`
	Class string `yaml:"class"`

	Tunnels   SectionTunnels   `yaml:"tunnels"`
	Transits  SectionTransits  `yaml:"transits"`
	Container SectionContainer `yaml:"container"`

	StartupWaittime uint32 `yaml:"startup-waittime"`

	NetworksDefaultIp6 []string `yaml:"networks-default-ip6"`
	NetworksDefaultIp4 []string `yaml:"networks-default-ip4"`
}

type SectionContainer struct {
	Types []string `yaml:"types"`
}

const (
	DEFAULT_NUMTXQUEUES = 1
	DEFAULT_NUMRXQUEUES = 1
	DEFAULT_TXQUEUELEN  = 1000
)

type SectionTunnels struct {
	DevicePrefix string `yaml:"device-prefix"`

	// proto could be "all", "xorp"
	// and so on to isolate table routes
	Proto string `yaml:"proto"`

	// Local address used as a local
	// endpoint for tunnel, is it possible
	// to detect it automatically? how
	// to filter all ip address to match
	// right one
	LocalAddress         string `yaml:"local-address"`
	LocalAddressResolved string `yaml:"local-address-resolved"`

	// encap-dport
	EncapDestPort uint32 `yaml:"encap-dport"`

	// Route table multiplication factor
	RouteTableMultiplier uint32 `yaml:"route-table-multiplier"`

	// Ecmp ip6 algorightm selection
	EcmpIp6Algo string `yaml:"ecmp-ip6-algo"`

	NumTxQueues uint32 `yaml:"numtxqueues"`
	NumRxQueues uint32 `yaml:"numrxqueues"`
	TxQueueLen  uint32 `yaml:"txqueuelen"`

	Bypass *Bypass `yaml:"bypass"`

	MplsRoutesMtu TMplsRoutesMtu `yaml:"mpls-routes-mtu"`
	TunnelsRoutes TTunnelsRoutes `yaml:"tunnels-routes"`
}

type TMplsRoutesMtu struct {
	Enabled bool `yaml:"enabled"`

	MtuDefaultIp6 uint32 `yaml:"mtu-default-ip6"`
	MtuDefaultIp4 uint32 `yaml:"mtu-default-ip4"`

	AdvmssDefaultIp6 uint32 `yaml:"advmss-default-ip6"`
	AdvmssDefaultIp4 uint32 `yaml:"advmss-default-ip4"`
}

type TTunnelsRoutes struct {
	Enabled   bool   `yaml:"enabled"`
	MtuIp6    uint32 `yaml:"mtu-ip6"`
	AdvmssIp6 uint32 `yaml:"advmss-ip6"`

	Device     string `yaml:"device"`
	GatewayIp6 string `yaml:"gateway-ip6"`

	// Actual value as device and gateway-ip6
	// could be an "auto" values. For each
	// container we need calculate values
	// from network
	DeviceValue     string `yaml:"device-value"`
	GatewayIp6Value string `yaml:"gateway-ip6-value"`
}

type SectionTransits struct {
	// proto could be "all", "xorp"
	// and so on to isolate table routes
	Proto string `yaml:"proto"`

	// Route table multiplication factor
	RouteTableMultiplier uint32 `yaml:"route-table-multiplier"`

	// Gateways
	GatewayIp4 string `yaml:"gateway-ip4"`
	GatewayIp6 string `yaml:"gateway-ip6"`

	// encap mpls routes should have a metric of '100'
	// while all the rest in specific table '1000'
	MetricEncap   uint32 `yaml:"metric-encap"`
	MetricDefault uint32 `yaml:"metric-default"`

	MtuDefaultIp6 uint32 `yaml:"mtu-default-ip6"`
	MtuDefaultIp4 uint32 `yaml:"mtu-default-ip4"`

	AdvmssDefaultIp6 uint32 `yaml:"advmss-default-ip6"`
	AdvmssDefaultIp4 uint32 `yaml:"advmss-default-ip4"`

	Overrides map[string]TransitsOverride `yaml:"overrides"`

	Bypass *Bypass `yaml:"bypass"`
}

// Some options could be overriden on per location basis
type TransitsOverride struct {
	// Gateways
	GatewayIp4 string `yaml:"gateway-ip4"`
	GatewayIp6 string `yaml:"gateway-ip6"`
}

func (t *TransitsOverride) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("gateway-ip4:'%s'", t.GatewayIp4))
	out = append(out, fmt.Sprintf("gateway-ip6:'%s'", t.GatewayIp6))

	return fmt.Sprintf("%s", strings.Join(out, ","))
}

type FilterItem struct {
	Enabled bool  `yaml:"enabled"`
	Links   []int `yaml:"links"`
}

func (f *FilterItem) AsString() string {
	var out []string
	out = append(out, fmt.Sprintf("enabled:'%t'", f.Enabled))

	var links []string
	for _, l := range f.Links {
		links = append(links, fmt.Sprintf("%d", l))
	}

	out = append(out, fmt.Sprintf("links:'[%s]'",
		strings.Join(links, ", ")))

	return fmt.Sprintf("%s", strings.Join(out, ","))
}

type SectionObjects struct {
	Links SectionLinks `yaml:"links"`
}

type SectionLinks struct {
	High   uint32                `yaml:"high"`
	Low    uint32                `yaml:"low"`
	Match  TMatch                `yaml:"match"`
	Filter map[string]FilterItem `yaml:"filter"`
}

type TMatch struct {
	Min int `yaml:"min"`
	Max int `yaml:"max"`
}

func (t *TMatch) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("min:'%d'", t.Min))
	out = append(out, fmt.Sprintf("max:'%d'", t.Max))

	return fmt.Sprintf("%s", strings.Join(out, ","))
}

func (s *SectionLinks) AsString() string {
	var out []string

	out = append(out, fmt.Sprintf("high:'%d'", s.High))
	out = append(out, fmt.Sprintf("low:'%d'", s.Low))

	for k, v := range s.Filter {
		out = append(out, fmt.Sprintf("k:'%s' -> v:'%s'",
			k, v.AsString()))
	}

	return fmt.Sprintf("%s", strings.Join(out, ", "))
}

type SectionM3 struct {
	Socket    string `yaml:"socket"`
	Port      int    `yaml:"port"`
	PidFile   string `yaml:"pidfile"`
	Logrotate bool   `yaml:"logrotate"`
	Source    string `yaml:"source"`
}

type SectionJuggler struct {
	Url     string `yaml:"url"`
	Enabled bool   `yaml:"enabled"`
}

type SectionSolomon struct {
	Endpoint string `yaml:"endpoint"`
	Project  string `yaml:"project"`
	Cluster  string `yaml:"cluster"`
	Token    string `yaml:"token"`
	Enabled  bool   `yaml:"enabled"`
}

// Classes of service: alpha (testing)
// and production
const (
	CLASS_UNKNOWN    = 0
	CLASS_ALPHA      = 101
	CLASS_PRODUCTION = 102
)

func ClassAsString(class int) string {
	classes := map[int]string{
		CLASS_UNKNOWN:    "class+unknown",
		CLASS_ALPHA:      "class+alpha",
		CLASS_PRODUCTION: "class+prod",
	}

	if _, ok := classes[class]; !ok {
		return classes[CLASS_UNKNOWN]
	}
	return classes[class]
}

const (
	ROLE_UNKNOWN          = 0
	ROLE_MPLS_TRANSIT     = 201
	ROLE_MPLS_UDP_TUNNELS = 202
)

func RoleAsString(code int) string {
	roles := map[int]string{
		ROLE_UNKNOWN:          "role+unknown",
		ROLE_MPLS_TRANSIT:     "role+mpls+transit",
		ROLE_MPLS_UDP_TUNNELS: "role+udp-tunnels",
	}
	return roles[code]
}

// Location code
const (
	LOC_UNKNOWN = 0
	LOC_SAS     = 301
	LOC_IVA     = 302
	LOC_MYT     = 303
	LOC_FOL     = 304
	LOC_VLA     = 305
	LOC_MAN     = 306
	LOC_MSKM9   = 307
	LOC_AMS     = 308

	LOC_MSKMAR       = 309
	LOC_MSKSTOREDATA = 310

	LOC_KIV = 311
	LOC_RAD = 312
	LOC_SPB = 313

	SOURCE_UNKNOWN = 0
	SOURCE_HTTP    = 101
	SOURCE_BGP     = 102
)

func SourceAsString(source int) string {
	sources := map[int]string{
		SOURCE_UNKNOWN: "unknown",
		SOURCE_HTTP:    "http",
		SOURCE_BGP:     "bgp",
	}

	if _, ok := sources[source]; ok {
		return sources[source]
	}

	return sources[SOURCE_UNKNOWN]
}

func LocationAsString(code int) string {
	locations := map[int]string{
		LOC_UNKNOWN:      "unknown",
		LOC_SAS:          "sas",
		LOC_IVA:          "iva",
		LOC_MYT:          "myt",
		LOC_FOL:          "fol",
		LOC_VLA:          "vla",
		LOC_MAN:          "man",
		LOC_MSKM9:        "mskm9",
		LOC_AMS:          "ams",
		LOC_MSKMAR:       "mskmar",
		LOC_MSKSTOREDATA: "mskstoredata",

		LOC_KIV: "kiv",
		LOC_RAD: "rad",
		LOC_SPB: "spb",
	}
	return locations[code]
}

// Runtime section
type SectionRuntime struct {
	Version string
	Date    string

	// Starting timestamp (for some monitoring events)
	T0 time.Time

	// Local bound ip addresses
	LocalIp string

	// Hostname, Type and Location settings
	// generated runtime
	Hostname string

	ProgramName string

	// Some self-configuration variables
	Class    int
	Role     int
	Location int

	Source int

	NetworkMode  int
	NetworkClass int
}

func (s *SectionRuntime) AsString() string {
	return fmt.Sprintf("hostname:'%s', class:'[%d]:[%s]', role:[%d]:[%s], location:[%d]:[%s] source:'%s'",
		s.Hostname, s.Class, ClassAsString(s.Class), s.Role,
		RoleAsString(s.Role), s.Location,
		LocationAsString(s.Location),
		SourceAsString(s.Source))
}

func LoadConf(confPath string, Overrides ConfigOverrides) (ConfYaml, error) {
	var conf ConfYaml

	viper.SetConfigType("yaml")
	viper.AutomaticEnv()
	viper.SetEnvPrefix("M3") // will be uppercased automatically
	viper.SetEnvKeyReplacer(strings.NewReplacer(".", "_"))

	var err error
	if confPath != "" {
		if _, err = os.Stat(confPath); err != nil {
			return conf, err
		}

		var content []byte
		if content, err = ioutil.ReadFile(confPath); err != nil {
			return conf, err
		}
		if err = viper.ReadConfig(bytes.NewBuffer(content)); err != nil {
			//fmt.Printf("using config file as: \"%s\"\n", viper.ConfigFileUsed())
			return conf, err
		}
	} else {
		// Search config in home directory with name "hhs"
		viper.AddConfigPath("/etc/m3")
		viper.AddConfigPath("$HOME/.m3")
		viper.AddConfigPath(".")
		viper.SetConfigName("m3")

		// If a config file is found, read it in.
		if err := viper.ReadInConfig(); err == nil {
			//fmt.Printf("using config file as: \"%s\"\n", viper.ConfigFileUsed())
		} else {
			// load default config
			if err := viper.ReadConfig(bytes.NewBuffer(defaultConf)); err != nil {
				return conf, err
			}
		}
	}

	conf.Overrides = &Overrides

	// logging parameters could be overriden by global
	// command line switches
	var LogOptions TLogOptions
	LogOptions.Format = viper.GetString("log.format")

	LogOptions.Level = viper.GetString("log.level")
	if Overrides.Debug {
		LogOptions.Level = "debug"
	}

	//  # "syslog" or "/var/log/m3/m3.log"
	//  # "stdout" (used in systemd startup case)
	//  log: "stdout"

	LogOptions.Log = viper.GetString("log.log")
	if len(Overrides.Log) > 0 {
		LogOptions.Log = Overrides.Log
	}

	conf.LogOptions = &LogOptions

	conf.Runtime.Hostname, _ = os.Hostname()
	conf.Runtime.T0 = time.Now()
	conf.Runtime.LocalIp = utils.GetLocalAddress()
	conf.Runtime.ProgramName = PROGRAM_NAME

	conf.Solomon.Enabled = viper.GetBool("solomon.enabled")
	conf.Solomon.Endpoint = viper.GetString("solomon.endpoint")
	conf.Solomon.Project = viper.GetString("solomon.project")
	conf.Solomon.Cluster = viper.GetString("solomon.cluster")
	conf.Solomon.Token = viper.GetString("solomon.token")

	conf.Juggler.Enabled = viper.GetBool("juggler.enabled")
	conf.Juggler.Url = viper.GetString("juggler.url")

	// TRAFFIC-11251 P.1. Processing token as file if
	// it sets in the form "file:/var/sec-01cz38q7s2azaeqc4jrcstg5v6.yav"
	tag := "file:"
	if strings.HasPrefix(conf.Solomon.Token, tag) {
		// Treat token part after "file:" as a file
		// and read content (if it exists)
		tags := strings.Split(conf.Solomon.Token, tag)
		if len(tags) > 1 && len(tags[1]) > 0 {
			conf.Solomon.Token = ""
			file := tags[1]
			var content []byte
			if content, err = ioutil.ReadFile(file); err == nil {
				conf.Solomon.Token = strings.TrimRight(string(content), "\n\r")
			}
		}
	}

	conf.M3.Socket = viper.GetString("m3.socket")
	conf.M3.PidFile = viper.GetString("m3.pidfile")
	conf.M3.Logrotate = viper.GetBool("m3.logrotate")
	conf.M3.Source = viper.GetString("m3.source")

	fqdn := conf.Runtime.Hostname
	conf.Runtime.Class, conf.Runtime.Role, conf.Runtime.Location,
		conf.Runtime.NetworkMode, conf.Runtime.NetworkClass =
		DetectEnvironment(fqdn)

	conf.Runtime.Source = SOURCE_BGP

	if strings.HasPrefix(conf.M3.Source, "http") {
		conf.Runtime.Source = SOURCE_HTTP
	}

	conf.Bgp.As = viper.GetUint32("bgp.as")
	conf.Bgp.PeerAs = viper.GetUint32("bgp.peer-as")
	conf.Bgp.RouterId = viper.GetString("bgp.router-id")
	conf.Bgp.DefaultSafi = viper.GetString("bgp.default-safi")

	if len(conf.Bgp.DefaultSafi) == 0 {
		conf.Bgp.DefaultSafi = DEFAULT_SAFI
	}

	conf.Bgp.Peers = viper.GetStringSlice("bgp.peers")

	conf.Bgp.RemotePort = viper.GetInt32("bgp.remote-port")
	conf.Bgp.LocalPort = viper.GetInt32("bgp.local-port")

	conf.Bgp.LocalAddress = viper.GetString("bgp.local-address")
	conf.Bgp.Passive = viper.GetBool("bgp.passive")
	conf.Bgp.Enabled = viper.GetBool("bgp.enabled")
	conf.Bgp.MessagesDumped = viper.GetBool("bgp.messages-dumped")

	conf.Bgp.Overrides = make(map[string]BgpOverrides)
	{
		m := viper.GetStringMap("bgp.overrides")
		for k, v := range m {
			var M BgpOverrides
			for t, w := range v.(map[string]interface{}) {
				switch t {
				case "peers":
					for _, p := range w.([]interface{}) {
						M.Peers = append(M.Peers, p.(string))
					}
				}
			}
			conf.Bgp.Overrides[k] = M
		}
	}

	loc := conf.Runtime.Location
	if w, ok := conf.Bgp.Overrides[LocationAsString(loc)]; ok {
		conf.Bgp.Peers = w.Peers
	}

	conf.Objects.Links.High = viper.GetUint32("objects.links.high")
	conf.Objects.Links.Low = viper.GetUint32("objects.links.low")

	viper.SetDefault("objects.links.match.min", 0)
	viper.SetDefault("objects.links.match.max", MaxInt)

	conf.Objects.Links.Match.Min = viper.GetInt("objects.links.match.min")
	conf.Objects.Links.Match.Max = viper.GetInt("objects.links.match.max")

	conf.Objects.Links.Filter = make(map[string]FilterItem)

	c := viper.GetStringMap("objects.links.filter")
	for k, v := range c {
		//fmt.Printf("class: '%s' v = '%+v' \n", k, v)
		// map[enabled:true links:[154 155 209]]
		var F FilterItem
		for t, w := range v.(map[string]interface{}) {
			//fmt.Printf("t:'%s' w:'%+v' \n", t, w)

			switch t {
			case "enabled":
				F.Enabled = w.(bool)
			case "links":
				for _, r := range w.([]interface{}) {
					F.Links = append(F.Links, r.(int))
				}
			}
		}
		conf.Objects.Links.Filter[k] = F
	}

	// global monitoring options
	conf.Monitoring.SessionWaittime = viper.GetUint32("monitoring.session-waittime")
	conf.Monitoring.SessionNeighbors = viper.GetUint32("monitoring.session-neighbors")
	conf.Monitoring.SessionPrefixes = viper.GetUint32("monitoring.session-prefixes")
	conf.Monitoring.SessionLinks = viper.GetUint32("monitoring.session-links")

	conf.Monitoring.Overrides = make(map[string]MonitoringOverride)
	m := viper.GetStringMap("monitoring.overrides")
	for k, v := range m {
		var M MonitoringOverride
		for t, w := range v.(map[string]interface{}) {
			switch t {
			case "session-neighbors":
				M.SessionNeighbors = uint32(w.(int))
			case "session-prefixes":
				M.SessionPrefixes = uint32(w.(int))
			case "session-links":
				M.SessionLinks = uint32(w.(int))
			}
		}
		conf.Monitoring.Overrides[k] = M
	}

	conf.Network.Class = viper.GetString("network.class")
	conf.Network.Mode = viper.GetString("network.mode")
	conf.Network.StartupWaittime = viper.GetUint32("network.startup-waittime")

	conf.Network.NetworksDefaultIp6 = viper.GetStringSlice("network.networks-default-ip6")
	conf.Network.NetworksDefaultIp4 = viper.GetStringSlice("network.networks-default-ip4")

	// Validating class and mode
	classes := []string{NETWORK_CLASS_TUNNELS_STR,
		NETWORK_CLASS_TRANSITS_STR}
	if !utils.StringInSlice(conf.Network.Class, classes) {
		err = errors.New(fmt.Sprintf("class:'%s' is incorrect, expecting one of ['%s']",
			conf.Network.Class, strings.Join(classes, ", ")))

		return conf, err
	}

	modes := []string{NETWORK_MODE_HOST_STR,
		NETWORK_MODE_CONTAINER_STR}
	if !utils.StringInSlice(conf.Network.Mode, modes) {
		err = errors.New(fmt.Sprintf("mode:'%s' is incorrect, expecting one of ['%s']",
			conf.Network.Mode, strings.Join(modes, ", ")))

		return conf, err
	}

	// if m3 could not detect class and role itself we use
	// configured values
	if conf.Runtime.Role == ROLE_UNKNOWN {

		if conf.Network.Mode == NETWORK_MODE_HOST_STR {
			conf.Runtime.NetworkMode = NETWORK_MODE_HOST
		}
		if conf.Network.Mode == NETWORK_MODE_CONTAINER_STR {
			conf.Runtime.NetworkMode = NETWORK_MODE_CONTAINER
		}
		if conf.Network.Mode == NETWORK_MODE_HOST_STR {
			conf.Runtime.NetworkMode = NETWORK_MODE_HOST
		}
		if conf.Network.Mode == NETWORK_MODE_CONTAINER_STR {
			conf.Runtime.NetworkMode = NETWORK_MODE_CONTAINER
		}

		if conf.Network.Class == NETWORK_CLASS_TUNNELS_STR {
			conf.Runtime.NetworkClass = NETWORK_CLASS_TUNNELS
			conf.Runtime.Role = ROLE_MPLS_UDP_TUNNELS
		}
		if conf.Network.Class == NETWORK_CLASS_TRANSITS_STR {
			conf.Runtime.NetworkClass = NETWORK_CLASS_TRANSITS
			conf.Runtime.Role = ROLE_MPLS_TRANSIT
		}
	}

	conf.Network.Tunnels.DevicePrefix =
		viper.GetString("network.tunnels.device-prefix")
	conf.Network.Tunnels.Proto =
		viper.GetString("network.tunnels.proto")
	conf.Network.Tunnels.LocalAddress =
		viper.GetString("network.tunnels.local-address")
	conf.Network.Tunnels.EncapDestPort =
		viper.GetUint32("network.tunnels.encap-dport")

	conf.Network.Tunnels.NumTxQueues =
		viper.GetUint32("network.tunnels.numtxqueues")
	conf.Network.Tunnels.NumRxQueues =
		viper.GetUint32("network.tunnels.numrxqueues")
	conf.Network.Tunnels.TxQueueLen =
		viper.GetUint32("network.tunnels.txqueuelen")

	if conf.Network.Tunnels.NumTxQueues == 0 {
		conf.Network.Tunnels.NumTxQueues = DEFAULT_NUMTXQUEUES
	}
	if conf.Network.Tunnels.NumRxQueues == 0 {
		conf.Network.Tunnels.NumRxQueues = DEFAULT_NUMRXQUEUES
	}
	if conf.Network.Tunnels.TxQueueLen == 0 {
		conf.Network.Tunnels.TxQueueLen = DEFAULT_TXQUEUELEN
	}

	conf.Network.Tunnels.MplsRoutesMtu.Enabled =
		viper.GetBool("network.tunnels.mpls-routes-mtu.enabled")

	conf.Network.Tunnels.MplsRoutesMtu.MtuDefaultIp6 =
		viper.GetUint32("network.tunnels.mpls-routes-mtu.mtu-default-ip6")

	conf.Network.Tunnels.MplsRoutesMtu.MtuDefaultIp4 =
		viper.GetUint32("network.tunnels.mpls-routes-mtu.mtu-default-ip4")

	conf.Network.Tunnels.MplsRoutesMtu.AdvmssDefaultIp6 =
		viper.GetUint32("network.tunnels.mpls-routes-mtu.advmss-default-ip6")

	conf.Network.Tunnels.MplsRoutesMtu.AdvmssDefaultIp4 =
		viper.GetUint32("network.tunnels.mpls-routes-mtu.advmss-default-ip4")

	conf.Network.Tunnels.TunnelsRoutes.Enabled =
		viper.GetBool("network.tunnels.tunnels-routes.enabled")

	conf.Network.Tunnels.TunnelsRoutes.MtuIp6 =
		viper.GetUint32("network.tunnels.tunnels-routes.mtu-ip6")

	conf.Network.Tunnels.TunnelsRoutes.AdvmssIp6 =
		viper.GetUint32("network.tunnels.tunnels-routes.advmss-ip6")

	conf.Network.Tunnels.TunnelsRoutes.Device =
		viper.GetString("network.tunnels.tunnels-routes.device")

	conf.Network.Tunnels.TunnelsRoutes.GatewayIp6 =
		viper.GetString("network.tunnels.tunnels-routes.gateway-ip6")

	conf.Network.Tunnels.RouteTableMultiplier =
		viper.GetUint32("network.tunnels.route-table-multiplier")

	conf.Network.Tunnels.EcmpIp6Algo =
		viper.GetString("network.tunnels.ecmp-ip6-algo")

	conf.Network.Tunnels.Bypass = ConfBypass("network.tunnels.bypass")

	conf.Network.Container.Types = viper.GetStringSlice("network.container.types")

	conf.Network.Transits.Proto =
		viper.GetString("network.transits.proto")

	conf.Network.Transits.RouteTableMultiplier =
		viper.GetUint32("network.transits.route-table-multiplier")

	conf.Network.Transits.GatewayIp4 =
		viper.GetString("network.transits.gateway-ip4")

	conf.Network.Transits.GatewayIp6 =
		viper.GetString("network.transits.gateway-ip6")

	conf.Network.Transits.MetricEncap =
		viper.GetUint32("network.transits.metric-encap")

	conf.Network.Transits.MetricDefault =
		viper.GetUint32("network.transits.metric-default")

	conf.Network.Transits.MtuDefaultIp6 =
		viper.GetUint32("network.transits.mtu-default-ip6")

	conf.Network.Transits.MtuDefaultIp4 =
		viper.GetUint32("network.transits.mtu-default-ip4")

	conf.Network.Transits.AdvmssDefaultIp6 =
		viper.GetUint32("network.transits.advmss-default-ip6")

	conf.Network.Transits.AdvmssDefaultIp4 =
		viper.GetUint32("network.transits.advmss-default-ip4")

	conf.Network.Transits.Overrides = make(map[string]TransitsOverride)
	{
		m := viper.GetStringMap("network.transits.overrides")
		for k, v := range m {
			var T TransitsOverride
			for t, w := range v.(map[string]interface{}) {
				switch t {
				case "gateway-ip4":
					T.GatewayIp4 = w.(string)
				case "gateway-ip6":
					T.GatewayIp6 = w.(string)
				}
			}
			conf.Network.Transits.Overrides[k] = T
		}
	}

	conf.Network.Transits.Bypass = ConfBypass("network.transits.bypass")

	conf.Lxd.Socket = viper.GetString("lxd.socket")

	return conf, nil
}

func (c *CmdGlobal) CreateSingletons(socket string) {

	// Assuming that all auth requests are done to internal
	// w.r.t CA certificate
	caCertPool := x509.NewCertPool()
	caCertPool.AppendCertsFromPEM(GetInternalCACertificate())
	tlsConfig := &tls.Config{
		RootCAs: caCertPool,
	}
	tlsConfig.BuildNameToCertificate()
	c.Transport = &http.Transport{TLSClientConfig: tlsConfig}

	tlsSkipConfig := &tls.Config{
		InsecureSkipVerify: true,
	}
	c.TransportSkipTls = &http.Transport{TLSClientConfig: tlsSkipConfig}

	c.TransportUnix = &http.Transport{
		DialContext: func(_ context.Context, _, _ string) (net.Conn, error) {
			return net.Dial("unix", socket)
		},
	}
}

// Function to detect properties for fqdn supplied
// returning codes for class, role, location
func DetectEnvironment(fqdn string) (int, int, int, int, int) {

	class := CLASS_UNKNOWN
	role := ROLE_UNKNOWN
	location := LOC_UNKNOWN
	mode := NETWORK_MODE_UNKNOWN
	networkclass := NETWORK_CLASS_UNKNOWN

	if fqdn == "alpha-20i.lxd.tt.yandex.net" {
		class = CLASS_ALPHA
		role = ROLE_MPLS_UDP_TUNNELS
		location = LOC_MAN
		mode = NETWORK_MODE_HOST
		networkclass = NETWORK_CLASS_TUNNELS
	}

	// TODO: environment autodiscovery
	if strings.HasPrefix(fqdn, "spb-srv") {
		class = CLASS_PRODUCTION
		role = ROLE_MPLS_UDP_TUNNELS
		location = LOC_SPB
		mode = NETWORK_MODE_CONTAINER
		networkclass = NETWORK_CLASS_TUNNELS

		return class, role, location, mode, networkclass
	}

	if strings.HasPrefix(fqdn, "mskmar-srv") {
		class = CLASS_PRODUCTION
		role = ROLE_MPLS_UDP_TUNNELS
		location = LOC_MSKMAR
		mode = NETWORK_MODE_CONTAINER
		networkclass = NETWORK_CLASS_TUNNELS

		return class, role, location, mode, networkclass
	}

	if strings.HasPrefix(fqdn, "mskm9-srv") {
		class = CLASS_PRODUCTION
		role = ROLE_MPLS_UDP_TUNNELS
		location = LOC_MSKM9
		mode = NETWORK_MODE_CONTAINER
		networkclass = NETWORK_CLASS_TUNNELS

		return class, role, location, mode, networkclass
	}

	if strings.HasPrefix(fqdn, "mskstoredata-srv") {
		class = CLASS_PRODUCTION
		role = ROLE_MPLS_UDP_TUNNELS
		location = LOC_MSKSTOREDATA
		mode = NETWORK_MODE_CONTAINER
		networkclass = NETWORK_CLASS_TUNNELS

		return class, role, location, mode, networkclass
	}

	if strings.HasPrefix(fqdn, "ams-srv") {
		class = CLASS_PRODUCTION
		role = ROLE_MPLS_UDP_TUNNELS
		location = LOC_AMS
		mode = NETWORK_MODE_CONTAINER
		networkclass = NETWORK_CLASS_TUNNELS

		return class, role, location, mode, networkclass
	}

	if strings.HasPrefix(fqdn, "kiv-srv") {
		class = CLASS_PRODUCTION
		role = ROLE_MPLS_UDP_TUNNELS
		location = LOC_KIV
		mode = NETWORK_MODE_CONTAINER
		networkclass = NETWORK_CLASS_TUNNELS

		return class, role, location, mode, networkclass
	}

	if strings.HasPrefix(fqdn, "rad-srv") {
		class = CLASS_PRODUCTION
		role = ROLE_MPLS_UDP_TUNNELS
		location = LOC_RAD
		mode = NETWORK_MODE_CONTAINER
		networkclass = NETWORK_CLASS_TUNNELS

		return class, role, location, mode, networkclass
	}

	if strings.HasSuffix(fqdn, "regions.yandex.net") {
		class = CLASS_PRODUCTION
		role = ROLE_MPLS_TRANSIT
		mode = NETWORK_MODE_CONTAINER
		networkclass = NETWORK_CLASS_TRANSITS

		if strings.HasPrefix(fqdn, "kiv-srv") {
			location = LOC_KIV
		}
		if strings.HasPrefix(fqdn, "rad-srv") {
			location = LOC_RAD
		}
	}

	return class, role, location, mode, networkclass
}

// Checking if count of parameters OK
func (c *CmdGlobal) CheckArgs(cmd *cobra.Command, args []string, minArgs int, maxArgs int) (bool, error) {
	if len(args) < minArgs || (maxArgs != -1 && len(args) > maxArgs) {
		cmd.Help()
		if len(args) == 0 {
			return true, nil
		}
		return true, fmt.Errorf("Invalid number of arguments")
	}
	return false, nil
}

func (c *CmdGlobal) LogDump(id string, content string) {
	rows := strings.Split(content, "\n")
	for i, r := range rows {
		if len(r) == 0 {
			continue
		}
		c.Log.Debug(fmt.Sprintf("%s [%02d]/[%02d] %s",
			id, i+1, len(rows), r))
	}
}

func (c *CmdGlobal) LogInfoDump(id string, content string) {
	rows := strings.Split(content, "\n")
	for i, r := range rows {
		c.Log.Info(fmt.Sprintf("%s [%02d]/[%02d] %s",
			id, i+1, len(rows), r))
	}
}

func GetInternalCACertificate() []byte {
	var caCert string = `-----BEGIN CERTIFICATE-----
MIIFZTCCA02gAwIBAgIKUlD06gAAAAAAGDANBgkqhkiG9w0BAQ0FADAfMR0wGwYD
VQQDExRZYW5kZXhJbnRlcm5hbFJvb3RDQTAeFw0xODA2MjgxMTE0NTdaFw0zMjA2
MjgxMTI0NTdaMFsxEjAQBgoJkiaJk/IsZAEZFgJydTEWMBQGCgmSJomT8ixkARkW
BnlhbmRleDESMBAGCgmSJomT8ixkARkWAmxkMRkwFwYDVQQDExBZYW5kZXhJbnRl
cm5hbENBMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAy6Sab1PCbISk
GSAUpr6JJKLXlf4O+cBhjALfQn2QpPL/cDjZ2+MPXuAUgE8KT+/mbAGA2rJID0KY
RjDSkByxnhoX8jwWsmPYXoAmOMPkgKRG9/ZefnMrK4oVhGgLmxnpbEkNbGh88cJ1
OVzgD5LVHSpDqm7iEuoUPOJCWXQ51+rZ0Lw9zBEU8v3yXXI345iWpLj92pOQDH0G
Tqr7BnQywxcgb5BYdywayacIT7UTJZk7832m5k7Oa3qMIKKXHsx26rNVUVBfpzph
OFvqkLetOKHk7827NDKr3I3OFXzQk4gy6tagv8PZNp+XGOBWfYkbLfI4xbTnjHIW
n5q1gfKPOQIDAQABo4IBZTCCAWEwEAYJKwYBBAGCNxUBBAMCAQIwIwYJKwYBBAGC
NxUCBBYEFNgaef9LcdQKs6qfsfiuWF5p/yqRMB0GA1UdDgQWBBSP3TKDCRNT3ZEa
Zumz1DzFtPJnSDBZBgNVHSAEUjBQME4GBFUdIAAwRjBEBggrBgEFBQcCARY4aHR0
cDovL2NybHMueWFuZGV4LnJ1L2Nwcy9ZYW5kZXhJbnRlcm5hbENBL3BvbGljaWVz
Lmh0bWwwGQYJKwYBBAGCNxQCBAweCgBTAHUAYgBDAEEwCwYDVR0PBAQDAgGGMA8G
A1UdEwEB/wQFMAMBAf8wHwYDVR0jBBgwFoAUq7nF/6Hv5lMdMzkihNF21DdOLWow
VAYDVR0fBE0wSzBJoEegRYZDaHR0cDovL2NybHMueWFuZGV4LnJ1L1lhbmRleElu
dGVybmFsUm9vdENBL1lhbmRleEludGVybmFsUm9vdENBLmNybDANBgkqhkiG9w0B
AQ0FAAOCAgEAQnOiyykjwtSuCBV6rSiM8Q1rQIcfyqn1JBxSGeBMABc64loWSPaQ
DtYPIW5rwNX7TQ94bjyYgCxhwHqUED/fcBOmXCQ2iBsdy5LOcNEZaC2kBHQuZ7dL
0fSvpE98a41y9yY6CJGFXg8E/4GrQwgQEqT5Qbe9GHPadpRu+ptVvI6uLZG3ks2o
oodjOm5C0SIo1pY4OtPAYE/AzTaYkTFbAqYcPfEfXHEOigBJBeXnQs7cANxX/RaF
PnHEjZbGY57EtBP6p5ckndkfEmqp3PLXbsQteNOVpsUw5eVqEzinSisBmLc28nnr
5QEojRontAaZd7ZzB5zaGkVuE+0laUUWSNBhfGE1R3LrTJEK9L7FEsBBprOxIWww
CvLmAfglouwuNRc2TjRdfnZaEfPLD7NYIF4ahXPAMcfTii23Tlr2uB7LetNykSlX
Z9S5/yf61VFEKnxuipFPNgtKqPcFgFUxlEb+wOeOfYZ7ex8VlpMBWbadj3Go025b
KZUwKwHDQvgJ5pz9g3t+t5Xieu2pwyddWGu+1SItRohRhlyTiep7oW6yTps7Qt0e
8pdLuLG7ZF19h1Pxi+dVbeaeNcsGEAOdRuCk+RTZHNe+J4yC8tNJOepnfYDul6SB
RjFWthiFK45+TZRHAcsG9JuV8JNvgoKaL75v/GUsKaeJ3Cps3rBStfc=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFGTCCAwGgAwIBAgIQJMM7ZIy2SYxCBgK7WcFwnjANBgkqhkiG9w0BAQ0FADAf
MR0wGwYDVQQDExRZYW5kZXhJbnRlcm5hbFJvb3RDQTAeFw0xMzAyMTExMzQxNDNa
Fw0zMzAyMTExMzUxNDJaMB8xHTAbBgNVBAMTFFlhbmRleEludGVybmFsUm9vdENB
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAgb4xoQjBQ7oEFk8EHVGy
1pDEmPWw0Wgw5nX9RM7LL2xQWyUuEq+Lf9Dgh+O725aZ9+SO2oEs47DHHt81/fne
5N6xOftRrCpy8hGtUR/A3bvjnQgjs+zdXvcO9cTuuzzPTFSts/iZATZsAruiepMx
SGj9S1fGwvYws/yiXWNoNBz4Tu1Tlp0g+5fp/ADjnxc6DqNk6w01mJRDbx+6rlBO
aIH2tQmJXDVoFdrhmBK9qOfjxWlIYGy83TnrvdXwi5mKTMtpEREMgyNLX75UjpvO
NkZgBvEXPQq+g91wBGsWIE2sYlguXiBniQgAJOyRuSdTxcJoG8tZkLDPRi5RouWY
gxXr13edn1TRDGco2hkdtSUBlajBMSvAq+H0hkslzWD/R+BXkn9dh0/DFnxVt4XU
5JbFyd/sKV/rF4Vygfw9ssh1ZIWdqkfZ2QXOZ2gH4AEeoN/9vEfUPwqPVzL0XEZK
r4s2WjU9mE5tHrVsQOZ80wnvYHYi2JHbl0hr5ghs4RIyJwx6LEEnj2tzMFec4f7o
dQeSsZpgRJmpvpAfRTxhIRjZBrKxnMytedAkUPguBQwjVCn7+EaKiJfpu42JG8Mm
+/dHi+Q9Tc+0tX5pKOIpQMlMxMHw8MfPmUjC3AAd9lsmCtuybYoeN2IRdbzzchJ8
l1ZuoI3gH7pcIeElfVSqSBkCAwEAAaNRME8wCwYDVR0PBAQDAgGGMA8GA1UdEwEB
/wQFMAMBAf8wHQYDVR0OBBYEFKu5xf+h7+ZTHTM5IoTRdtQ3Ti1qMBAGCSsGAQQB
gjcVAQQDAgEAMA0GCSqGSIb3DQEBDQUAA4ICAQAVpyJ1qLjqRLC34F1UXkC3vxpO
nV6WgzpzA+DUNog4Y6RhTnh0Bsir+I+FTl0zFCm7JpT/3NP9VjfEitMkHehmHhQK
c7cIBZSF62K477OTvLz+9ku2O/bGTtYv9fAvR4BmzFfyPDoAKOjJSghD1p/7El+1
eSjvcUBzLnBUtxO/iYXRNo7B3+1qo4F5Hz7rPRLI0UWW/0UAfVCO2fFtyF6C1iEY
/q0Ldbf3YIaMkf2WgGhnX9yH/8OiIij2r0LVNHS811apyycjep8y/NkG4q1Z9jEi
VEX3P6NEL8dWtXQlvlNGMcfDT3lmB+tS32CPEUwce/Ble646rukbERRwFfxXojpf
C6ium+LtJc7qnK6ygnYF4D6mz4H+3WaxJd1S1hGQxOb/3WVw63tZFnN62F6/nc5g
6T44Yb7ND6y3nVcygLpbQsws6HsjX65CoSjrrPn0YhKxNBscF7M7tLTW/5LK9uhk
yjRCkJ0YagpeLxfV1l1ZJZaTPZvY9+ylHnWHhzlq0FzcrooSSsp4i44DB2K7O2ID
87leymZkKUY6PMDa4GkDJx0dG4UXDhRETMf+NkYgtLJ+UIzMNskwVDcxO4kVL+Hi
Pj78bnC5yCw8P5YylR45LdxLzLO68unoXOyFz1etGXzszw8lJI9LNubYxk77mK8H
LpuQKbSbIERsmR+QqQ==
-----END CERTIFICATE-----`
	return []byte(caCert)
}
