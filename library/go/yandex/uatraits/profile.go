package uatraits

import (
	"crypto/md5"
	"encoding/hex"
	"encoding/xml"
	"io/ioutil"

	"golang.org/x/xerrors"
)

type profileDefine struct {
	XMLName xml.Name `xml:"define"`
	Name    string   `xml:"name,attr"`
	Value   string   `xml:"value,attr"`
}

type profile struct {
	XMLName xml.Name        `xml:"profile"`
	URL     string          `xml:"url,attr"`
	ID      string          `xml:"id,attr"` // actually ID stores MD5 hash of URL
	Defines []profileDefine `xml:"define"`
}

type profilesStorage struct {
	XMLName  xml.Name  `xml:"profiles"`
	Profiles []profile `xml:"profile"`
}

func getMD5Hash(value string) string {
	hash := md5.Sum([]byte(value))
	return hex.EncodeToString(hash[:])
}

func (storage profilesStorage) Trigger(headerValue string, traits Traits) {
	id := getMD5Hash(headerValue)
	for _, profile := range storage.Profiles {
		if profile.ID == id {
			for _, define := range profile.Defines {
				traits[define.Name] = define.Value
			}
		}
	}
}

func parseProfileRulesXMLBytes(bytes []byte) (*profilesStorage, error) {
	storage := &profilesStorage{}
	if err := xml.Unmarshal(bytes, storage); err != nil {
		return nil, xerrors.Errorf("cannot parse profiles: %w", err)
	}
	return storage, nil
}

func parseProfilesStorage(profilesXMLFilePath string) (*profilesStorage, error) {
	bytes, err := ioutil.ReadFile(profilesXMLFilePath)
	if err != nil {
		return nil, xerrors.Errorf("cannot read profiles XML: %w", err)
	}

	return parseProfileRulesXMLBytes(bytes)
}
