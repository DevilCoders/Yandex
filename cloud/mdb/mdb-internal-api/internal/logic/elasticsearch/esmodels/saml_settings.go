package esmodels

import (
	"encoding/xml"
	"net/url"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

type SamlSettings struct {
	IDPEntityID string
	IDPMetadata []byte

	SPEntityID string
	KibanaURL  string

	Principal string
	Groups    string
	Name      string
	Mail      string
	DN        string
}

func (s SamlSettings) validate() error {
	if err := s.validateIDPEntityID(); err != nil {
		return err
	}
	if err := s.validateIDPMetadata(); err != nil {
		return err
	}
	if err := s.validateSPEntityID(); err != nil {
		return err
	}
	if err := s.validateKibanaURL(); err != nil {
		return err
	}
	if err := s.validateAttributes(); err != nil {
		return err
	}

	return nil
}

func (s SamlSettings) validateIDPEntityID() error {
	id := s.IDPEntityID

	if id == "" {
		return semerr.InvalidInput("idp_entity_id is required")
	}
	if len(id) > 1024 {
		return semerr.InvalidInput("max length of idp_entity_id is 1024 characters")
	}

	return nil
}

func (s SamlSettings) validateIDPMetadata() error {
	m := s.IDPMetadata

	if len(m) == 0 {
		return semerr.InvalidInput("idp_metadata_file is required")
	}

	ed := new(struct {
		XMLName  xml.Name `xml:"EntityDescriptor"`
		EntityID string   `xml:"entityID,attr"`
	})
	err := xml.Unmarshal(m, ed)
	if err != nil {
		return semerr.InvalidInput("idp_metadata_file is not an xml file")
	}
	if ed.EntityID != s.IDPEntityID {
		return semerr.InvalidInput("entityID from idp_metadata_file doesn't match with idp_entity_id")
	}

	return nil
}

func (s SamlSettings) validateSPEntityID() error {
	if s.SPEntityID == "" {
		return semerr.InvalidInput("sp_entity_id is required")
	}

	return nil
}

func (s SamlSettings) validateKibanaURL() error {
	ku := s.KibanaURL

	if ku == "" {
		return semerr.InvalidInput("kibana_url is required")
	}

	u, err := url.Parse(ku)
	if err != nil || u.Scheme == "" || u.Host == "" {
		return semerr.InvalidInput("kibana_url isn't a url")
	}

	return nil
}

func (s SamlSettings) validateAttributes() error {
	if s.Principal == "" {
		return semerr.InvalidInput("attribute_principal is required")
	}

	return nil
}
