package utils

import (
	"fmt"
	"net"
)

// GetLocalAddress gets local ip address
func GetLocalAddress() string {
	out := ""
	ifaces, err := net.Interfaces()
	if err != nil {
		return out
	}

	for _, i := range ifaces {
		addrs, err := i.Addrs()
		if err != nil {
			continue
		}

		for _, addr := range addrs {
			var ip net.IP
			switch v := addr.(type) {
			case *net.IPNet:
				ip = v.IP
			case *net.IPAddr:
				ip = v.IP
			}

			if ip.IsGlobalUnicast() {
				out = fmt.Sprintf("%s", ip)
			}
		}
	}

	return out
}
