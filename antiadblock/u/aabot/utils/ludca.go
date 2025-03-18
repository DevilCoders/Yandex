package main

import (
	"encoding/base64"
	"fmt"
	"strings"
)

const splitter = "dmVyc2lvbg"
const versionSplitter = "bHVkY2E="
const versionSplitterNew = "bHVkY2E"

// func main() {
// 	fmt.Println(1, decode("azE5N3h4YzA=bHVkY2E=PFRdG1hIURAkUk0XSkhSCUsACA1MTlkCUhF+eixyNn4gf3ZgNnIxVQ1ESlIcWBdfS1RPVhQNAkQOEVgXCwwRWQVWGVYLWClRHVBqVAoRE0RLU1xUGQ0QVUsWTFkLGQVVRlRPVhRfQ1kYEVdYDFgCXktQVVsXDwZUS0JWQgobBhAEVxlEGwoKQB8RUFlYDAtVS1dWWxQXFFkFVhl0FxYXVQVFGWQdGxZCAkVAFygXD1kISBlTEQoGUx9YT1JCWEFDCENQRwxVEEIIER5EHRQFF0tTVVgaQkMXHl9KVh4dTlkFXVBZHV9DWB9FSURCV0xDHlZeUgsMTl0KQUoZARkNVA5JF0UNWAtEH0FKDVdXGlEFVVxPVgoWH0tZTUMIC1kfREJMUB8dEERGXExbDBFNSQpfXVIAVg1VHxFRQwwIEApEHkBWCwwCRAJSF1kdDENYH0VJREJXTFEFH0BWFhwGSEVDTBcQDBdAGAsWGBkcEB4KVV9YAFYRRUtZTUMIC1kfRFBdRE5WAlQNXkEZCg1DWB9FSURCV0xJCl9dUgBWEERLWU1DCAtZH0RTShkBGQ1UDkkXRQ1YC0QfQUoNV1cCQAIcVFYIC01JCl9dUgBWEUVLWU1DCAtZH0QbF1oZCBAeElBXUx0ATV4ORRlfDAwTQ1EeFlobVhpRBVVcT1YKFhADRU1HC0JMHxxUW1YLCk1JCl9dUgBWDVUfEVFDDAgQCkQeSVYLC01JCl9dUgBWEUVLFkpfGUpWBkZSFkQqHSVGGH5vbxc0DV0laQwHLTNVZFloS1FXLCgGPntuAh4KVmlZdAl6RV9DFxhZWAVNTk5UClJvXRlLFWoyRRYHNSJIfwh/DU8RThF3BwJPRAwdN2kSflpAMT5WezNgBBBYXxBYCgMMAVU6NEZSYn50ExE0WAgEXXwhAA5YBwlxYRQqLFgzGggAMzAadwMaAW8QPAdTVhYZEAsQAgJeBxRvFVcoXiEEfV02SAlfH0hVW1cyAkIlYHF6STYAZyRWfWENKyJIRHJAfi8XXhdLFkpfGUpWBkZ3Q10hAShVP3tQQjFXAko4YUFcFAECCDl8d14MLQ9ALlhhRjEABlI5QG9YRV9DFxhZWAVNTk5jGVNRUCkyFkY4WlFgIh4KQCV7aHIaFhF2GgZ2AxE6DggofH97Hk0lQRNoBBBYXxBYCgMMAVUrAmQbdFtDNFMQfxFwegZAKA5bRHdedCEoKB8+S11mEB0VVC1LUVtMQTtbVhYZEAsQAgJeBxRiOQkMe1pCXUAXClBZOQQWWC43WlQhdXEFTx4aHygHUgYyKg1nIGZLVBwhXhdLFkpfGUpWBkZbWmc+FAFRBHN3dVc9AmkRfV9vASo3fS1LSFxMLCtpGVJ1YzVJGVYRBlhERV9DFxhZWAVNTk52PwF4fwAaE1gPQHdfAk1SQRkaeg4WGyZqAGVzXzshU14ja1B6KzshdQAFBBBYXxBYCgMMAVVKUl4yeQsHEEshAAdJamAhIQlDPkVAD0A/O1FaVBJiVykKdA9yC01IHAZHVhYZEBYXDVMOHEt1EzESHwVDSG8aEkhKEX0OYU41GXFWDB4XXxYMXghUFEECPQZlUmUIZT5MMGU/WFNGSQgNVDoMBBBYEBdEG0IDGFcBAkMfUE0ZFh0XEANFTUcLQkwfElBKQxkMTUIeExc9"))

// 	fmt.Println(2, decode("2AjvdmVyc2lvbg6ibfENnx7KL5fC82Um9P77TuW3r2ywTNRTNEInPsDmw6mgUCvagtvX6798v4HrH7RDEu-VlKiKFzPUZaEVQN_AbHVkY2Ewr1Dwrs8w7nDgcOfwoLCth9bFmBffsOXwpTDn2lAw4fDsj7DvXITA28nw4xrAF_Dt2Bsw4nCiBHDmRfDjcOXwrjCjGfDncKeeRNawpYpcMKowoxCDXYqaW8twpDCj0DCqyrDucOcw53CksOJDFcNcgcqwobDk8KGL0DDlsO8McK9PQhkVRrCiHoEAMK6MjfDjcOQFsKdEcONwpLCucKecsOewox-EUbCkD0uw63Dj0gdNjViPXnClcKFSMOlMMK4wpPCn8ONwpUJW1NyTibCgsOEwoEpDsKXwqVww7ZnDXhGGsKaLgBbw7hgbsKAworRrdO-0L_TutON07PTjT7SgNK40b7Qg9Glw5t5KcOkw4AATnt4YSZiwprCjE_Cq2TCrcKFwpjDlsKNCA0WOwtyw43Dq8K8djPDm8OpJMK-MUooR07DjmoFScOqaWPDhMKSDcOTEcOVwpLDqcOGIsKewp8tRxDDhXYuw6HDl00"))

// 	fmt.Println(3, decode("6ibfENnx7KL5fC82Um9P77TuW3r2ywTNRTNEInPsDmw6mgUCvagtvX6798v4HrH7RDEu-VlKiKFzPUZaEVQN_A==bHVkY2E=p0mxPPnD24K4CUgWYF9-15TfaEDG8z75cxMDbyfMawBf92BsyYgR2RfN17iMZ92eeRNalilwqIxCDXYqaW8tkI9Aqyr53N2SyQxXDXIHKobThi9A1vwxvT0IZFUaiHoEALoyN83QFp0RzZK5nnLejH4RRpA9Lu3PSB02NWI9eZWFSOUwuJOfzZUJW1NyTiaCxIEpDpelcPZnDXhGGpouD1b7dnGAil3PEd2RooxqxY8wRVqNe2rhxU4fGQg8HSDeylWrabWU0YCdFVxGPg421ZSANBST6TrxalctVE3QIQhT7Ds"))
// 	fmt.Println(4, decode("a256cDh5ZGM=bHVkY2E=PxsfXBhLU0MqHghQCklWUkteTUoJQF5RXk49PWxzMS0gIDUndnM8KzlOCBVJDAEQH04fAkoWFmk4GhsETQpeQ2EGDgRICl5MRBcbHlwcHE0ZG1UeXQ4XTA=="))
// }

func xor(input, key string) (output string) {
	for i := 0; i < len(input); i++ {
		output += string(input[i] ^ key[i%len(key)])
	}

	return output
}

func utf8Decode(input []byte) string {
	result := make([]byte, 0)

	for i := 0; i < len(input); {
		char := input[i]

		if char < 128 {
			result = append(result, char)
			i++
		} else {
			if char > 191 && char < 224 {
				char2 := input[i+1]
				result = append(result, ((char&31)<<6)|(char2&63))
				i += 2
			} else {
				char2 := input[i+1]
				char3 := input[i+2]
				result = append(result, ((char&15)<<12)|((char2&63)<<6)|(char3&63))
				i += 3
			}
		}
	}

	return string(result)
}

func decodeBase64(input string, useDecoder bool) string {
	if m := len(input) % 4; m != 0 {
		input += strings.Repeat("=", 4-m)
	}

	encoder := base64.StdEncoding

	i := strings.Index(input, "+")
	j := strings.Index(input, "/")
	if i < 0 && j < 0 {
		encoder = base64.URLEncoding
	}

	data, err := encoder.DecodeString(input)
	if err != nil {
		fmt.Println("error:", err)
		return ""
	}

	var result string

	if useDecoder {
		result = utf8Decode(data)
	} else {
		result = string(data)
	}

	return result
}

func decode(ludca string) string {
	vs := versionSplitter
	words := strings.Split(ludca, splitter)
	useDecoder := false

	// Новая версия людки
	if len(words) != 1 {
		vs = versionSplitterNew
		ludca = words[1]
		useDecoder = true
	}

	words = strings.Split(ludca, vs)

	key := decodeBase64(words[0], false)
	data := decodeBase64(words[1], useDecoder)

	result := xor(string(data), string(key))

	return result
}
