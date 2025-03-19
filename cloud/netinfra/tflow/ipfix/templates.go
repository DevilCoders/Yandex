package ipfix

import (
	"encoding/binary"
	"io"
	"net"
)

type tmpl struct {
	ID         uint16
	FieldCount uint16
	Len        uint16
	Fields     []uint16
}

type IP46Addr [16]byte

type domainKey struct {
	Exporter IP46Addr //net.IP can't be part of a map key :(
	DomainID uint32
}

func MakeIP46Addr(addr net.IP) IP46Addr {
	var retval IP46Addr
	copy(retval[:], addr.To16())
	return retval
}

func newTemplate(id uint16, count uint16) tmpl {
	return tmpl{id, count, 0, make([]uint16, count)}
}

func parseTemplate(r io.Reader) (tmpl, []elt, error) {
	var id, count uint16
	elts := make([]elt, 0, count)
	err := binary.Read(r, binary.BigEndian, &id)
	if err != nil {
		return tmpl{}, nil, err
	}
	err = binary.Read(r, binary.BigEndian, &count)
	if err != nil {
		return tmpl{}, nil, err
	}
	template := newTemplate(id, count)
	var i uint16
	for i = 0; i < count; i++ {
		var eltID, eltLen uint16
		err = binary.Read(r, binary.BigEndian, &eltID)
		if err != nil {
			return tmpl{}, nil, err
		}
		err = binary.Read(r, binary.BigEndian, &eltLen)
		if err != nil {
			return tmpl{}, nil, err
		}
		eltID &= eltIDMask
		if !isElementKnown(eltID) {
			e := newUnknownElement(eltID, eltLen)
			elts = append(elts, e)
		}
		template.Fields[i] = eltID
		template.Len += eltLen
	}
	return template, elts, nil
}

func registerTemplate(addr IP46Addr, domain uint32, template tmpl) {
	key := domainKey{addr, domain}
	if _, ok := templates[key]; !ok {
		templates[key] = map[uint16]tmpl{}
	}
	templates[key][template.ID] = template
}

func removeTemplate(addr IP46Addr, domain uint32, id uint16) {
	key := domainKey{addr, domain}
	if _, ok := templates[key]; ok {
		delete(templates[key], id)
	}
}

func isTemplateKnown(addr IP46Addr, domain uint32, id uint16) bool {
	key := domainKey{addr, domain}
	if _, ok := templates[key]; ok {
		_, ok := templates[key][id]
		return ok
	}
	return false
}

func getTemplate(addr IP46Addr, domain uint32, id uint16) tmpl {
	key := domainKey{addr, domain}
	return templates[key][id]
}

func areTemplatesEqual(left, right tmpl) bool {
	if left.ID != right.ID ||
		left.FieldCount != right.FieldCount ||
		left.Len != right.Len {
		return false
	}
	var i uint16
	for i = 0; i < left.FieldCount; i++ {
		if left.Fields[i] != right.Fields[i] {
			return false
		}
	}
	return true
}

func dropExporter(addr IP46Addr, domain uint32) {
	key := domainKey{addr, domain}
	delete(templates, key)
}

var templates = map[domainKey]map[uint16]tmpl{}
var eltIDMask uint16 = 0x7fff
var templateHeaderLen uint16 = 4
var templateFieldLen uint16 = 4
