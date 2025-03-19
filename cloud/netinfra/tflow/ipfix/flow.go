package ipfix

import (
	"bytes"
	"encoding/binary"
	"io"
	"strings"

	"a.yandex-team.ru/cloud/netinfra/tflow/consumer"
)

type flowset struct {
	ID  uint16
	Len uint16
}

type dataflow struct {
	Header   string
	Elements []element
}

func (d dataflow) String() string {
	var builder strings.Builder
	builder.WriteString(d.Header)
	for _, e := range d.Elements {
		builder.WriteString("\t")
		builder.WriteString(e.Name)
		builder.WriteString("=")
		builder.WriteString(e.Value)
	}
	builder.WriteString("\n")
	return builder.String()
}

// see https://www.iana.org/assignments/ipfix/ipfix.xhtml#ipfix-set-ids
var templateFlowSetID uint16 = 2
var flowSetHeaderLen uint16 = 4

func isTemplateFlowSet(fset *flowset) bool {
	return fset.ID == templateFlowSetID
}

func newDataFlow(count uint16, hdr string) dataflow {
	return dataflow{hdr, make([]element, count)}
}

func parseDataFlow(r io.Reader, template *tmpl, hdr string) (dataflow, error) {
	var i uint16
	flow := newDataFlow(template.FieldCount, hdr)
	for i = 0; i < template.FieldCount; i++ {
		var err error
		flow.Elements[i], err = decodeElement(template.Fields[i], r)
		if err != nil {
			return dataflow{}, err
		}
	}
	return flow, nil
}

func parseFlowSetHeader(r io.Reader) (flowset, error) {
	var id, len uint16
	if err := binary.Read(r, binary.BigEndian, &id); err != nil {
		return flowset{}, err
	}
	if err := binary.Read(r, binary.BigEndian, &len); err != nil {
		return flowset{}, err
	}
	return flowset{id, len}, nil
}

func parseDataFlowSet(data []byte, e ipfixExtra) ([]consumer.MessageElement, error) {
	if !isTemplateKnown(e.addr, e.domain, e.fset.ID) {
		// unknown template id. Just skip flowset
		return nil, nil
	}
	t := getTemplate(e.addr, e.domain, e.fset.ID)
	flowCount := (e.fset.Len - flowSetHeaderLen) / t.Len
	retval := make([]consumer.MessageElement, flowCount)
	r := bytes.NewReader(data[flowSetHeaderLen:])
	var i uint16
	for i = 0; i < flowCount; i++ {
		var err error
		retval[i], err = parseDataFlow(r, &t, e.header)
		if err != nil {
			return nil, err
		}
	}
	return retval, nil
}

func parseTemplateFlowSet(r io.Reader, fset flowset) ([]tmpl, []elt, error) {
	tmpls := make([]tmpl, 0, fset.Len/templateFieldLen)
	elts := make([]elt, 0, fset.Len/templateFieldLen)
	data := make([]byte, fset.Len-flowSetHeaderLen)
	if _, err := r.Read(data); err != nil {
		return nil, nil, err
	}
	rawTemplates := bytes.NewReader(data)
	var pos uint16
	for pos = 0; pos < fset.Len-flowSetHeaderLen; {
		template, elements, err := parseTemplate(rawTemplates)
		if err != nil {
			return nil, nil, err
		}
		tmpls = append(tmpls, template)
		elts = append(elts, elements...)
		pos += templateHeaderLen + templateFieldLen*template.FieldCount
	}
	return tmpls, elts, nil
}
