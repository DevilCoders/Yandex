package core

import (
	"fmt"
	"net"
	"strconv"
	"strings"

	"github.com/miekg/dns"
)

type Answer struct {

	// Data that was resolved
	Data string `json:"data"`

	// Code of resolution: could
	// be NXDOMAIN, NOERROR, SERVFAIL
	Code int `json:"code"`

	Nameserver string   `json:"nameserver"`
	Records    []string `json:"records"`
}

func (a *Answer) AsString() string {
	return fmt.Sprintf("data:'%s' code:'%d' msg:'%s', count:'%d' records:['%s'] via '%s'",
		a.Data, a.Code, dns.RcodeToString[a.Code], len(a.Records),
		strings.Join(a.Records, ","), a.Nameserver)
}

func Resolve(nameserver string, qname string, qtype uint16) (Answer, error) {
	var answer Answer

	port := 53

	if len(nameserver) == 0 {
		conf, err := dns.ClientConfigFromFile("/etc/resolv.conf")
		if err != nil {
			return answer, err
		}
		nameserver = "@" + conf.Servers[0]
	}

	nameserver = string([]byte(nameserver)[1:]) // chop off @
	// if the nameserver is from /etc/resolv.conf the [ and ] are already
	// added, thereby breaking net.ParseIP. Check for this and don't
	// fully qualify such a name
	if nameserver[0] == '[' && nameserver[len(nameserver)-1] == ']' {
		nameserver = nameserver[1 : len(nameserver)-1]
	}
	if i := net.ParseIP(nameserver); i != nil {
		nameserver = net.JoinHostPort(nameserver, strconv.Itoa(port))
	} else {
		nameserver = dns.Fqdn(nameserver) + ":" + strconv.Itoa(port)
	}

	d := new(dns.Client)
	d.Net = "udp"
	m := new(dns.Msg)
	m.RecursionDesired = true

	q := dns.Fqdn(qname)
	m.SetQuestion(q, qtype)

	r, _, err := d.Exchange(m, nameserver)
	if err != nil {
		return answer, err

	}

	answer.Code = r.Rcode
	answer.Data = qname
	answer.Nameserver = nameserver

	for _, r := range r.Answer {
		switch z := r.(type) {
		case *dns.NS:
			ns := fmt.Sprintf("%s", z.Ns)
			if qtype == dns.TypeNS {
				answer.Records = append(answer.Records, ns)
			}
		case *dns.AAAA:
			ip6 := fmt.Sprintf("%s", z.AAAA)
			if qtype == dns.TypeAAAA {
				answer.Records = append(answer.Records, ip6)
			}
		case *dns.A:
			ip := fmt.Sprintf("%s", z.A)
			if qtype == dns.TypeA {
				answer.Records = append(answer.Records, ip)
			}
		case *dns.CNAME:
			target := fmt.Sprintf("%s", z.Target)
			if qtype == dns.TypeCNAME {
				answer.Records = append(answer.Records, target)
			}
		case *dns.TXT:
			txt := fmt.Sprintf("%s", strings.Join(z.Txt, ""))
			if qtype == dns.TypeTXT {
				answer.Records = append(answer.Records, txt)
			}
		case *dns.PTR:
			ptr := fmt.Sprintf("%s", z.Ptr)
			if qtype == dns.TypePTR {
				answer.Records = append(answer.Records, ptr)
			}
		}
	}

	return answer, nil
}
