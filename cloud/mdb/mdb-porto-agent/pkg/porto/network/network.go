package network

import (
	"crypto/md5"
	"encoding/hex"
	"fmt"
	"net"
	"strings"

	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/porto"
	l "a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const managedPrefix = "managed-"

var _ porto.Network = &Network{}

// Network support IP address hints for container
type Network struct {
	log l.Logger
}

// New create new dom0 network implementation
func New(log l.Logger) *Network {
	return &Network{
		log: log,
	}
}

// GetExpectedIPAddrs return IPs hint for container
func (pn *Network) GetExpectedIPAddrs(container, projectID, managingProjectID string, useVLAN688 bool) (string, error) {
	commonIP, err := pn.getExpectedIPAddr(container, projectID, useVLAN688)
	if err != nil {
		return "", xerrors.Errorf("get common IP: %w", err)
	}

	if managingProjectID != "" && useVLAN688 {
		managingIP, err := pn.getExpectedIPAddr(managedPrefix+container, managingProjectID, true)
		if err != nil {
			return "", xerrors.Errorf("get managing IP: %w", err)
		}

		return fmt.Sprintf("eth0 %s;eth0 %s", commonIP, managingIP), nil
	}

	return fmt.Sprintf("eth0 %s", commonIP), nil
}

// GetExpectedIPAddr return IP hint for container
func (pn *Network) getExpectedIPAddr(container, projID string, useVLAN688 bool) (net.IP, error) {
	if useVLAN688 {
		v688, err := net.InterfaceByName("vlan688")
		if err != nil {
			return nil, porto.ErrNoExpectedInterfaceAddr.Wrap(xerrors.Errorf("no vlan688 interface: %w", err))
		}
		return pn.getExpectedIPAddrForIface(container, projID, v688)
	}

	ifs, err := net.Interfaces()
	if err != nil {
		return nil, porto.ErrNoExpectedInterfaceAddr.Wrap(err)
	}
	for _, iface := range ifs {
		if !strings.HasPrefix(iface.Name, "eth") {
			pn.log.Debugf("ignore, because %s is not eth iface", iface.Name)
			continue
		}
		ip, err := pn.getExpectedIPAddrForIface(container, projID, &iface)
		if err != nil {
			pn.log.Debugf("expected iface address search error %s", err)
		}
		if ip != nil {
			return ip, err
		}
	}
	return nil, porto.ErrNoExpectedInterfaceAddr.Wrap(xerrors.Errorf("expected IP not found in interface addr enumeration"))
}

func (pn *Network) getExpectedIPAddrForIface(container, projID string, iface *net.Interface) (net.IP, error) {
	addrs, err := iface.Addrs()
	if err != nil {
		return nil, porto.ErrNoExpectedInterfaceAddr.Wrap(xerrors.Errorf("%s interface, no iface addrs, %s", iface.Name, err))
	}
	var network string
	for _, addr := range addrs {
		as := addr.String()
		if strings.HasPrefix(as, "fe80") {
			continue
		}
		pas := strings.Split(as, ":")
		if len(pas) < 4 {
			pn.log.Debugf("getExpectedIPAddrForIface, iface %s, skip address %s", iface.Name, as)
			continue
		}
		network = strings.Join(pas[:4], ":")
		break
	}
	if network == "" {
		return nil, porto.ErrNoExpectedInterfaceAddr.Wrap(xerrors.Errorf("%s interface, no expected network, available addresses: %v", iface.Name, addrs))
	}

	dgst := md5.Sum([]byte(container))
	tailDgst := dgst[12:]
	tail := fmt.Sprintf("%s:%s", hex.EncodeToString(tailDgst[:2]), hex.EncodeToString(tailDgst[2:]))
	return net.ParseIP(fmt.Sprintf("%s:%s:%s", network, projID, tail)), nil
}
