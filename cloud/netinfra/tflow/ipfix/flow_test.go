package ipfix

import (
	"bytes"
	"net"
	"testing"
)

func TestParseDataFlow(t *testing.T) {
	var tmplID uint16 = 256
	template := newTemplate(tmplID, 8)
	template.Fields = []uint16{323, 230, 8, 225, 4, 7, 227, 234}
	data := []byte{
		0x00, 0x00, 0x01, 0x6c, 0x1e, 0x3e, 0xeb, 0xfb,
		0x04, 0xc0, 0xa8, 0x00, 0x01, 0x0a, 0x00, 0x00,
		0x01, 0x01, 0x05, 0x6b, 0xcc, 0xc6, 0x00, 0x00,
		0x00, 0x00,
	}
	means := []element{
		{"observationTimeMilliseconds", "2019.07.23 09:52:15.867 UTC"},
		{"natEvent", "NAT44 session create"},
		{"sourceIPv4Address", "192.168.0.1"},
		{"postNATSourceIPv4Address", "10.0.0.1"},
		{"protocolIdentifier", "icmp"},
		{"sourceTransportPort", "1387"},
		{"postNATSourceTransportPort", "52422"},
		{"ingressVRFID", "0"},
	}
	result, _ := parseDataFlow(bytes.NewReader(data), &template, "")
	if len(result.Elements) != len(means) {
		t.Errorf("got      %v\n", result.Elements)
		t.Errorf("expected %v\n", means)
		t.Fatalf("got %d fields expected %d", len(result.Elements), len(means))
	}
	for i, elt := range result.Elements {
		if elt.Name != means[i].Name || elt.Value != means[i].Value {
			t.Errorf("element #%d got %s:%s expected %s:%s",
				i, elt.Name, elt.Value, means[i].Name, means[i].Value)
		}
	}

}

func TestParseFlowSetHeader(t *testing.T) {
	data := []byte{0, 42, 0x0f, 0xed}
	result, _ := parseFlowSetHeader(bytes.NewReader(data))
	if result.ID != 42 {
		t.Errorf("got id %d expected 42", result.ID)
	}
	if result.Len != 4077 {
		t.Errorf("got len %d expected 4077", result.Len)
	}
}

func TestParseDataFlowSet(t *testing.T) {
	var addr IP46Addr = MakeIP46Addr(net.ParseIP("10.0.0.1"))
	var domain uint32 = 4077
	var tmplID uint16 = 256
	var dataSetLen uint16 = 82
	fset := flowset{tmplID, dataSetLen}
	e := ipfixExtra{addr, domain, fset, ""}
	data := []byte{
		0x01, 0x00, 0x00, 0x52,
		0x00, 0x00, 0x01, 0x6c, 0x1e, 0x3e, 0xeb, 0xfb,
		0x04, 0xc0, 0xa8, 0x00, 0x01, 0x0a, 0x00, 0x00,
		0x01, 0x01, 0x05, 0x6b, 0xcc, 0xc6, 0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00, 0x01, 0x6c, 0x1e, 0x3e, 0xf3, 0x7c,
		0x04, 0xc0, 0xa8, 0x00, 0x01, 0x0a, 0x00, 0x00,
		0x01, 0x01, 0x05, 0x6f, 0xbd, 0xdd, 0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00, 0x01, 0x6c, 0x1e, 0x3e, 0xfb, 0x41,
		0x04, 0xc0, 0xa8, 0x00, 0x01, 0x0a, 0x00, 0x00,
		0x01, 0x01, 0x05, 0x73, 0x9e, 0xaf, 0x00, 0x00,
		0x00, 0x00,
	}
	means := []dataflow{
		{"", []element{
			{"observationTimeMilliseconds", "Tue Jul 23 12:52:15 MSK 2019"},
			{"natEvent", "NAT44 session create"},
			{"sourceIPv4Address", "192.168.0.1"},
			{"postNATSourceIPv4Address", "10.0.0.1"},
			{"protocolIdentifier", "icmp"},
			{"sourceTransportPort", "1387"},
			{"postNATSourceTransportPort", "52422"},
			{"ingressVRFID", "0"},
		}},
		{"", []element{
			{"observationTimeMilliseconds", "Tue Jul 23 12:52:17 MSK 2019"},
			{"natEvent", "NAT44 session create"},
			{"sourceIPv4Address", "192.168.0.1"},
			{"postNATSourceIPv4Address", "10.0.0.1"},
			{"protocolIdentifier", "icmp"},
			{"sourceTransportPort", "1391"},
			{"postNATSourceTransportPort", "48605"},
			{"ingressVRFID", "0"},
		}},
		{"", []element{
			{"observationTimeMilliseconds", "Tue Jul 23 12:52:19 MSK 2019"},
			{"natEvent", "NAT44 session create"},
			{"sourceIPv4Address", "192.168.0.1"},
			{"postNATSourceIPv4Address", "10.0.0.1"},
			{"protocolIdentifier", "icmp"},
			{"sourceTransportPort", "1395"},
			{"postNATSourceTransportPort", "40623"},
			{"ingressVRFID", "0"},
		}},
	}
	// init template to parse with
	var fieldCount uint16 = 8
	template := newTemplate(tmplID, fieldCount)
	template.Fields = []uint16{323, 230, 8, 225, 4, 7, 227, 234}
	template.Len = 26
	registerTemplate(addr, domain, template)
	defer removeTemplate(addr, domain, template.ID)

	retval, _ := parseDataFlowSet(data, e)
	if len(retval) != len(means) {
		t.Fatalf("got %d flows expected %d", len(retval), len(means))
		for i, f := range retval {
			if f.String() != means[i].String() {
				t.Errorf("flow %d got %s expected %s", i, f, means[i].String())
			}
		}
	}
}

func TestParseTemplateFlowSet(t *testing.T) {
	data := []byte{
		0x01, 0x00, 0x00, 0x08, 0x01, 0x43, 0x00, 0x08,
		0x00, 0xe6, 0x00, 0x01, 0x00, 0x08, 0x00, 0x04,
		0x00, 0xe1, 0x00, 0x04, 0x00, 0x04, 0x00, 0x01,
		0x00, 0x07, 0x00, 0x02, 0x00, 0xe3, 0x00, 0x02,
		0x00, 0xea, 0x00, 0x04,
	}
	fset := flowset{templateFlowSetID, 40}
	means := newTemplate(256, 8)
	means.Fields = []uint16{323, 230, 8, 225, 4, 7, 227, 234}
	tmpls, elts, _ := parseTemplateFlowSet(bytes.NewReader(data), fset)
	if len(elts) != 0 {
		t.Errorf("found unknown fields while should not")
	}
	if len(tmpls) != 1 {
		t.Fatalf("failed to parse template")
	}
	for i, f := range tmpls[0].Fields {
		if f != means.Fields[i] {
			t.Errorf("got field %d wanted %d", f, means.Fields[i])
		}
	}
}
