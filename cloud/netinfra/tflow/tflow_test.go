package main

import (
	"bufio"
	"github.com/google/gopacket"
	"github.com/google/gopacket/layers"
	"github.com/google/gopacket/pcap"
	"net"
	"os"
	"sort"
	"strings"
	"testing"
	"time"
)

var (
	CONF_FILE      = "/tflow_test.conf"
	PCAP_FILE      = "/tflow_test.pcap"
	LOG_FILE       = "/tflow_test.log"
	PID_FILE       = "/tflow_test.pid"
	SFLOW_FILE     = "/tflow_test_samples.log"
	ENCAP_FILE     = "/tflow_test_encap.log"
	NAT_FILE       = "/tflow_test_nat.log"
	SFLOW_REF_FILE = "/tflow_test_samples_reference.log"
	ENCAP_REF_FILE = "/tflow_test_encap_reference.log"
	NAT_REF_FILE   = "/tflow_test_nat_reference.log"
)

type Line struct {
	Pairs map[string]string
}

func newLine() Line {
	return Line{make(map[string]string)}
}

func (l Line) Len() int {
	return len(l.Pairs)
}

func (l Line) Eq(r Line) bool {
	for key, value := range l.Pairs {
		if key == "unixtime" {
			continue
		}
		if value != r.Pairs[key] {
			return false
		}
	}
	return true
}

func (l Line) setPair(key, value string) {
	l.Pairs[key] = value
}

func Test(t *testing.T) {
	dir, err := os.Getwd()
	if err != nil {
		t.Fatalf("failed to get current dir")
	}
	defer func() {
		os.Remove(dir + LOG_FILE)
		os.Remove(dir + PID_FILE)
		os.Remove(dir + SFLOW_FILE)
		os.Remove(dir + ENCAP_FILE)
		os.Remove(dir + NAT_FILE)
	}()
	os.Args = []string{
		"tflow",
		"-config", dir + CONF_FILE,
		"-log-file", dir + LOG_FILE,
		"-pid-file", dir + PID_FILE,
		"-sflow-log", dir + SFLOW_FILE,
		"-encap-log", dir + ENCAP_FILE,
		"-ipfix-nat-log", dir + NAT_FILE,
	}
	go main()
	//give main some time to start and get ready
	time.Sleep(time.Second)
	feedPackets(dir+PCAP_FILE, t)
	//give main some time to finish processing
	time.Sleep(time.Second)
	p, err := os.FindProcess(os.Getpid())
	if err != nil {
		p.Signal(os.Interrupt)
	}
	//give main some time to terminate
	time.Sleep(1 * time.Second)
	compare(dir+SFLOW_FILE, dir+SFLOW_REF_FILE, t)
	compare(dir+ENCAP_FILE, dir+ENCAP_REF_FILE, t)
}

func compare(leftFilename string, rightFilename string, t *testing.T) {
	left := load(leftFilename, t)
	right := load(rightFilename, t)
	if len(left) != len(right) {
		t.Fatalf("line count doesnt match")
	}
	for i, line := range left {
		if !line.Eq(right[i]) {
			t.Errorf("line mismatch '%v' and '%v'", line, right[i])
		}
	}
}

func load(name string, t *testing.T) []Line {
	file, err := os.Open(name)
	if err != nil {
		t.Fatalf("failed to open file %v: %v", name, err)
	}
	defer file.Close()

	var lines []string
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := scanner.Text()
		lines = append(lines, line)
	}
	if scanner.Err() != nil {
		t.Fatalf("scanner error: %v", scanner.Err())
	}
	sort.Strings(lines)
	retval := []Line{}
	for _, line := range lines {
		pairs := strings.Split(line, "\t")
		rv := newLine()
		for _, pair := range pairs {
			tokens := strings.Split(pair, "=")
			if tokens[0] != "tskv" {
				rv.setPair(tokens[0], tokens[1])
			}
		}
		retval = append(retval, rv)
	}
	return retval
}

func feedPackets(fileName string, t *testing.T) {
	ServerAddr, err := net.ResolveUDPAddr("udp", "[::1]:6343")
	if err != nil {
		t.Fatalf("failed to resolve server: %v\n", err)
	}

	LocalAddr, err := net.ResolveUDPAddr("udp", "[::1]:0")
	if err != nil {
		t.Fatalf("failed to resolve client: %v\n", err)
	}

	Conn, err := net.DialUDP("udp", LocalAddr, ServerAddr)
	if err != nil {
		t.Fatalf("failed to dial: %v\n", err)
	}

	defer Conn.Close()

	if handle, err := pcap.OpenOffline(fileName); err != nil {
		t.Fatalf("failed to read pcap: %v\n", err)
	} else {
		packetSource := gopacket.NewPacketSource(handle, handle.LinkType())
		for packet := range packetSource.Packets() {
			appLayer := packet.Layer(layers.LayerTypeUDP)
			_, err := Conn.Write(appLayer.LayerPayload())
			if err != nil {
				t.Fatalf("failed to send packet: %v\n", err)
			}
		}
	}
}
