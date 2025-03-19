package ipfix

import (
	"a.yandex-team.ru/cloud/netinfra/tflow/consumer"
	"a.yandex-team.ru/cloud/netinfra/tflow/worker"
	"a.yandex-team.ru/library/go/core/log"
	"bytes"
	"fmt"
	"io"
)

type ipfixExtra struct {
	addr   IP46Addr
	domain uint32
	fset   flowset
	header string
}

func WorkerFunc(j worker.Job, out chan consumer.Message, logger log.Logger) {
	extra := j.Extra.(ipfixExtra)
	retval, err := parseDataFlowSet(j.Payload.Body, extra)
	if err != nil {
		logger.Errorf("ipfix worker func: %v\n", err)
		return
	}
	out <- consumer.Message{Type: consumer.IpfixNatMessage, Elements: retval}
}

func CollectorFunc(j worker.Job, wp chan *worker.Worker, logger log.Logger) {
	r := bytes.NewReader(j.Payload.Body)
	pkt, err := parsePacketHeader(r)
	addr := MakeIP46Addr(j.Payload.Raddr.IP)
	if err != nil {
		//log illformed header, ignore packet
		logger.Errorf("ipfix collector func parsePacketHeader: %v\n", err)
		j.Teardown()
		return
	}
	for offset := ipfixHeaderLen; offset < pkt.Length; {
		fset, err := parseFlowSetHeader(r)
		eos := offset + fset.Len
		if err != nil {
			//log illformed flowset, ignore source
			logger.Errorf("ipfix collector func parseFlowSetHeader: %v\n", err)
			dropExporter(addr, pkt.Domain)
			j.Teardown()
			return
		}
		if isTemplateFlowSet(&fset) {
			err := processTemplateFlowSet(r, addr, pkt.Domain, fset, wp)
			if err != nil {
				//log illformed flowset, ignore source
				logger.Errorf("ipfix collector func processTemplateFlowSet: %v\n", err)
				dropExporter(addr, pkt.Domain)
				j.Teardown()
				return
			}
			if eos == pkt.Length {
				j.Teardown()
			}
		} else {
			var job *worker.Job
			if eos == pkt.Length {
				//last flow set in datagram. We can use original job
				job = &j
				job.Payload.Body = job.Payload.Body[offset:eos]
			} else {
				job = worker.NewChildJob(j, offset, eos)
			}
			//TODO: possibly verbose header needed
			job.Extra = ipfixExtra{addr, pkt.Domain, fset, "tskv"}
			w := <-wp
			w.JobChannel <- *job
		}
		_, err = r.Seek(int64(eos), io.SeekStart)
		if err != nil {
			panic(fmt.Sprintf("ipfix collector func r.Seek: %v", err))
		}
		offset = eos
	}
}

func processTemplateFlowSet(
	r io.Reader,
	addr IP46Addr,
	domain uint32,
	fset flowset,
	wp chan *worker.Worker) error {

	tmpls, elts, err := parseTemplateFlowSet(r, fset)
	if err != nil {
		return err
	}
	needReconfig := false
	newTmpls := make([]tmpl, 0, len(tmpls))
	for _, tmpl := range tmpls {
		if !isTemplateKnown(addr, domain, tmpl.ID) {
			newTmpls = append(newTmpls, tmpl)
			needReconfig = true
			continue
		}
		t := getTemplate(addr, domain, tmpl.ID)
		if !areTemplatesEqual(tmpl, t) {
			newTmpls = append(newTmpls, tmpl)
			needReconfig = true
		}
	}
	if needReconfig {
		//wait for workers to become idle
		for len(wp) < cap(wp) {
		}
		for _, elt := range elts {
			registerElement(elt)
		}
		for _, tmpl := range newTmpls {
			registerTemplate(addr, domain, tmpl)
		}
	}
	return nil
}
