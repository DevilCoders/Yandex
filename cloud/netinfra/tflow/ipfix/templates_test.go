package ipfix

import (
	"bytes"
	"net"
	"testing"
)

func TestParseTemplate(t *testing.T) {
	data := []byte{
		1, 0, // template id
		0, 8, // field count
		1, 0x43, 0, 8, // ts
		0, 0xe6, 0, 1, // nat event
		0, 8, 0, 4, // src addr
		0, 0xe1, 0, 4, // post nat src addr
		0, 4, 0, 1, // proto
		0, 7, 0, 2, // src port
		0, 0xe3, 0, 2, // post nat src port
		0, 0xea, 0, 4, // vrf id
	}
	template, elts, _ := parseTemplate(bytes.NewReader(data))
	if template.ID != 256 {
		t.Errorf("template id got %d expected 256", template.ID)
	}
	if template.FieldCount != 8 {
		t.Errorf("field count got %d expected 8", template.FieldCount)
	}
	if template.Len != 26 {
		t.Errorf("length got %d expected 26", template.Len)
	}
	if len(elts) != 0 {
		t.Errorf("we've created elements %v while should not", elts)
	}
	exp_fields := []uint16{323, 230, 8, 225, 4, 7, 227, 234}
	var i uint16
	for i = 0; i < template.FieldCount; i++ {
		if template.Fields[i] != exp_fields[i] {
			t.Errorf("field #%d got %d expected %d", i, template.Fields[i], exp_fields[i])
		}
	}
}

func TestParseUnknownElements(t *testing.T) {
	data := []byte{1, 0, 0, 1, 0x0f, 0xed, 0, 1}
	_, elts, _ := parseTemplate(bytes.NewReader(data))
	if len(elts) != 1 {
		t.Fatalf("failed to find unknown element")
	}
	if elts[0].ID != 4077 {
		t.Errorf("got elt #%d expected %d", elts[0].ID, 4077)
	}
}

func TestRegisterTemplate(t *testing.T) {
	var addr IP46Addr = MakeIP46Addr(net.ParseIP("10.0.0.1"))
	var domain uint32 = 42
	var id uint16 = 4077
	if isTemplateKnown(addr, domain, id) {
		t.Fatalf("template #%d already known", id)
	}
	registerTemplate(addr, domain, newTemplate(id, 0))
	if !isTemplateKnown(addr, domain, id) {
		t.Fatalf("failed to register template #%d", id)
	}
	removeTemplate(addr, domain, id)
	if isTemplateKnown(addr, domain, id) {
		t.Fatalf("failed to remove template #%d", id)
	}
}

func TestAreTemplatesEqual(t *testing.T) {
	var left, right tmpl
	left = newTemplate(42, 2)
	left.Fields = []uint16{40, 77}
	left.Len = 8
	right = newTemplate(42, 2)
	right.Fields = []uint16{40, 77}
	right.Len = 8
	if !areTemplatesEqual(left, right) {
		t.Errorf("found difference in equal templates")
	}
	right.ID = 4077
	if areTemplatesEqual(left, right) {
		t.Errorf("found no difference in id")
	}
	right.ID = 42
	right.FieldCount = 3
	if areTemplatesEqual(left, right) {
		t.Errorf("found no difference in field count")
	}
	right.FieldCount = 2
	right.Len = 4
	if areTemplatesEqual(left, right) {
		t.Errorf("found no difference in length")
	}
	right.Len = 8
	right.Fields[1] = 78
	if areTemplatesEqual(left, right) {
		t.Errorf("found no difference in fields")
	}
}

func TestDropExporter(t *testing.T) {
	var addr IP46Addr = MakeIP46Addr(net.ParseIP("10.0.0.1"))
	var domain uint32 = 42
	var id uint16 = 4077
	registerTemplate(addr, domain, newTemplate(id, 0))
	if !isTemplateKnown(addr, domain, id) {
		t.Fatalf("failed to register template #%d", id)
	}
	dropExporter(addr, domain)
	if isTemplateKnown(addr, domain, id) {
		t.Fatalf("template known after exporter drop")
	}
}
