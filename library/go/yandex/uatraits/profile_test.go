package uatraits

import (
	"encoding/xml"
	"testing"

	"github.com/stretchr/testify/assert"
)

func createProfileDefine(name, value string) profileDefine {
	return profileDefine{
		XMLName: xml.Name{Local: "define"},
		Name:    name,
		Value:   value,
	}
}

func createProfile(url, id string, defines ...profileDefine) profile {
	return profile{
		XMLName: xml.Name{Local: "profile"},
		URL:     url,
		ID:      id,
		Defines: defines[:],
	}
}

func createProfilesStorage(profiles ...profile) profilesStorage {
	return profilesStorage{
		XMLName:  xml.Name{Local: "profiles"},
		Profiles: profiles[:],
	}
}

func TestParseProfilesStorage(t *testing.T) {
	profilesXMLBytes := []byte(`
<?xml version="1.0" encoding="UTF-8"?>
	<profiles>
	<profile url="http://122.200.68.229/docs/mini3ix.xml" id="7d80b53134b8d7b1610cb3a5b3314183">
		<define name="DeviceKeyboard" value="SoftKeyPad"/>
		<define name="DeviceModel" value="Dell_Mini3iX"/>
	</profile>
	<profile url="http://218.249.47.94/Xianghe/MTK_Athens15_UAProfile.xml" id="297202bf0b4e70a850ee14843b8d1df0">
		<define name="ScreenWidth" value="480"/>
		<define name="ScreenHeight" value="800"/>
	</profile>
</profiles>`)
	expectedProfilesStorage := createProfilesStorage(
		createProfile("http://122.200.68.229/docs/mini3ix.xml", "7d80b53134b8d7b1610cb3a5b3314183",
			createProfileDefine("DeviceKeyboard", "SoftKeyPad"),
			createProfileDefine("DeviceModel", "Dell_Mini3iX"),
		),
		createProfile("http://218.249.47.94/Xianghe/MTK_Athens15_UAProfile.xml", "297202bf0b4e70a850ee14843b8d1df0",
			createProfileDefine("ScreenWidth", "480"),
			createProfileDefine("ScreenHeight", "800"),
		),
	)

	actualProfilesStorage, err := parseProfileRulesXMLBytes(profilesXMLBytes)
	if !assert.NoError(t, err) {
		assert.FailNow(t, "cannot parse profiles", "%w", err)
	}
	assert.Equal(t, &expectedProfilesStorage, actualProfilesStorage)
}
