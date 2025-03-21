#include "synnorm.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/str.h>
#include <util/system/tempfile.h>
#include <library/cpp/string_utils/base64/base64.h>

using namespace NSynNorm;

const char* data =
"AwAAAM9i8CuYOwqRAgpMCiZjb250cmliL2xpYnMvcHJvdG9idWYvZGVzY3JpcHRvci5wcm90bxIg"
"MDNjOTlmMGM5OTQ2YjBiYzM0N2EyYjM2N2M1NTdiMjAYAQo/ChlkaWN0L2dhemV0dGVlci9iYXNl"
"LnByb3RvEiBjNDRkZGY1OWRmNjkxNzVkZDliM2RkYWQ5NThlYjk4NxgBClAKKnF1YWxpdHkvZ2xv"
"YmFsX3NlYXJjaC9zeW5ub3JtL3N5bnNldC5wcm90bxIgMjYyNmRjMTExZjBiZmEyYTJiZTgxYzhl"
"YWI4ZTM5NmMYAAouCgh0ZXN0Lmd6dBIgZTNjZDNjNGE1MzdjNzRlZDUwMTMyMGFiYTU1YjVkMDgY"
"ABKBNwoPVE1vcnBoTWF0Y2hSdWxlChVUQ2FwaXRhbGl6YXRpb25GaWx0ZXIKC1RHcmFtbWVtU2V0"
"Cg9UR3JhbW1lbXNGaWx0ZXIKDlRHbGVpY2hlRmlsdGVyCg9UTGFuZ3VhZ2VGaWx0ZXIKEFRUb2tl"
"bml6ZU9wdGlvbnMKClRTZWFyY2hLZXkKBFRSZWYKCFRBcnRpY2xlChxUR3p0T3B0aW9ucy5UV2ls"
"ZENhcmRPcHRpb25zCgtUR3p0T3B0aW9ucwohZ29vZ2xlLnByb3RvYnVmLkZpbGVEZXNjcmlwdG9y"
"U2V0CiNnb29nbGUucHJvdG9idWYuRmlsZURlc2NyaXB0b3JQcm90bwouZ29vZ2xlLnByb3RvYnVm"
"LkRlc2NyaXB0b3JQcm90by5FeHRlbnNpb25SYW5nZQofZ29vZ2xlLnByb3RvYnVmLkRlc2NyaXB0"
"b3JQcm90bwokZ29vZ2xlLnByb3RvYnVmLkZpZWxkRGVzY3JpcHRvclByb3RvCiNnb29nbGUucHJv"
"dG9idWYuRW51bURlc2NyaXB0b3JQcm90bwooZ29vZ2xlLnByb3RvYnVmLkVudW1WYWx1ZURlc2Ny"
"aXB0b3JQcm90bwomZ29vZ2xlLnByb3RvYnVmLlNlcnZpY2VEZXNjcmlwdG9yUHJvdG8KJWdvb2ds"
"ZS5wcm90b2J1Zi5NZXRob2REZXNjcmlwdG9yUHJvdG8KG2dvb2dsZS5wcm90b2J1Zi5GaWxlT3B0"
"aW9ucwoeZ29vZ2xlLnByb3RvYnVmLk1lc3NhZ2VPcHRpb25zChxnb29nbGUucHJvdG9idWYuRmll"
"bGRPcHRpb25zChtnb29nbGUucHJvdG9idWYuRW51bU9wdGlvbnMKIGdvb2dsZS5wcm90b2J1Zi5F"
"bnVtVmFsdWVPcHRpb25zCh5nb29nbGUucHJvdG9idWYuU2VydmljZU9wdGlvbnMKHWdvb2dsZS5w"
"cm90b2J1Zi5NZXRob2RPcHRpb25zCixnb29nbGUucHJvdG9idWYuVW5pbnRlcnByZXRlZE9wdGlv"
"bi5OYW1lUGFydAojZ29vZ2xlLnByb3RvYnVmLlVuaW50ZXJwcmV0ZWRPcHRpb24KEE5TeW5Ob3Jt"
"LlRTeW5zZXQKEE5TeW5Ob3JtLlRNZW1iZXISIAABAgMEBQYHCAkKCwwNDg8QERITFBUWFxgZGhsc"
"HR4JGsMRChlkaWN0L2dhemV0dGVlci9iYXNlLnByb3RvIoIBCg9UTW9ycGhNYXRjaFJ1bGUSJAoE"
"dHlwZRgBIAIoDjIWLlRNb3JwaE1hdGNoUnVsZS5FVHlwZRIMCgR3b3JkGAIgAygNIjsKBUVUeXBl"
"Eg4KCkVYQUNUX0ZPUk0QABINCglBTExfRk9STVMQARITCg9BTExfTEVNTUFfRk9STVMQAiLcAQoV"
"VENhcGl0YWxpemF0aW9uRmlsdGVyEjUKBWFsbG93GAEgAygOMiYuVENhcGl0YWxpemF0aW9uRmls"
"dGVyLkVDYXBpdGFsaXphdGlvbhI2CgZmb3JiaWQYAiADKA4yJi5UQ2FwaXRhbGl6YXRpb25GaWx0"
"ZXIuRUNhcGl0YWxpemF0aW9uEgwKBHdvcmQYAyADKA0iRgoPRUNhcGl0YWxpemF0aW9uEgcKA0FO"
"WRAAEgkKBUVYQUNUEAESCQoFTE9XRVIQAhIJCgVVUFBFUhADEgkKBVRJVExFEAQiGAoLVEdyYW1t"
"ZW1TZXQSCQoBZxgBIAMoCSJaCg9UR3JhbW1lbXNGaWx0ZXISGwoFYWxsb3cYASADKAsyDC5UR3Jh"
"bW1lbVNldBIcCgZmb3JiaWQYAiADKAsyDC5UR3JhbW1lbVNldBIMCgR3b3JkGAMgAygNIvsCCg5U"
"R2xlaWNoZUZpbHRlchIjCgR0eXBlGAEgAygOMhUuVEdsZWljaGVGaWx0ZXIuRVR5cGUSDAoEd29y"
"ZBgCIAMoDSK1AgoFRVR5cGUSCAoEQ0FTRRABEgoKBkdFTkRFUhACEgoKBk5VTUJFUhADEgkKBVRF"
"TlNFEAQSCgoGUEVSU09OEAUSFQoRU1VCSkVDVF9QUkVESUNBVEUQBhIWChJHRU5ERVJfTlVNQkVS"
"X0NBU0UQBxIPCgtOVU1CRVJfQ0FTRRAIEhEKDUdFTkRFUl9OVU1CRVIQCRIPCgtHRU5ERVJfQ0FT"
"RRAKEgwKCEZFTV9DQVNFEAsSCQoFY19hZ3IQZRIJCgVnX2FnchBmEgkKBW5fYWdyEGcSCQoFdF9h"
"Z3IQaBIJCgVwX2FnchBpEgoKBnNwX2FnchBqEgsKB2duY19hZ3IQaxIKCgZuY19hZ3IQbBIKCgZn"
"bl9hZ3IQbRIKCgZnY19hZ3IQbhINCglmZW1fY19hZ3IQbyLGBAoPVExhbmd1YWdlRmlsdGVyEiUK"
"BWFsbG93GAEgAygOMhYuVExhbmd1YWdlRmlsdGVyLkVMYW5nEiYKBmZvcmJpZBgCIAMoDjIWLlRM"
"YW5ndWFnZUZpbHRlci5FTGFuZxIMCgR3b3JkGAMgAygNItUDCgVFTGFuZxIHCgNVTksQABIHCgNS"
"VVMQARIHCgNFTkcQAhIHCgNQT0wQAxIHCgNIVU4QBBIHCgNVS1IQBRIHCgNHRVIQBhIHCgNGUk4Q"
"BxIHCgNUQVQQCBIHCgNCTFIQCRIHCgNLQVoQChIHCgNBTEIQCxIHCgNTUEEQDBIHCgNJVEEQDRIH"
"CgNBUk0QDhIHCgNEQU4QDxIHCgNQT1IQEBIHCgNTTE8QEhIHCgNTTFYQExIHCgNEVVQQFBIHCgNC"
"VUwQFRIHCgNDQVQQFhIHCgNIUlYQFxIHCgNDWkUQGBIHCgNHUkUQGRIHCgNIRUIQGhIHCgNOT1IQ"
"GxIHCgNNQUMQHBIHCgNTV0UQHRIHCgNLT1IQHhIHCgNMQVQQHxINCglCQVNJQ19SVVMQIBIHCgNG"
"SU4QJxIHCgNFU1QQKBIHCgNMQVYQKRIHCgNMSVQQKhIHCgNCQUsQKxIHCgNUVVIQLBIHCgNSVU0Q"
"LRIHCgNNT04QLhIHCgNVWkIQLxIHCgNLSVIQMBIHCgNUR0sQMRIHCgNUVUsQMhIHCgNTUlAQMxIH"
"CgNBWkUQNBINCglCQVNJQ19FTkcQNRIHCgNHRU8QNhIHCgNBUkEQNxIHCgNQRVIQOCKAAQoQVFRv"
"a2VuaXplT3B0aW9ucxIzCgVkZWxpbRgBIAEoDjIXLlRUb2tlbml6ZU9wdGlvbnMuRVR5cGU6C1dI"
"SVRFX1NQQUNFIjcKBUVUeXBlEg8KC1dISVRFX1NQQUNFEAASDQoJTk9OX0FMTlVNEAESDgoKQ0hB"
"Ul9DTEFTUxACIr4CCgpUU2VhcmNoS2V5EgwKBHRleHQYASADKAkSHwoFbW9ycGgYAiADKAsyEC5U"
"TW9ycGhNYXRjaFJ1bGUSHgoEZ3JhbRgDIAMoCzIQLlRHcmFtbWVtc0ZpbHRlchIkCgRjYXNlGAQg"
"AygLMhYuVENhcGl0YWxpemF0aW9uRmlsdGVyEhwKA2FnchgFIAMoCzIPLlRHbGVpY2hlRmlsdGVy"
"EioKBHR5cGUYBiABKA4yFy5UU2VhcmNoS2V5LkVLZXlDb250ZW50OgNLRVkSHgoEbGFuZxgHIAMo"
"CzIQLlRMYW5ndWFnZUZpbHRlchIjCgh0b2tlbml6ZRgIIAEoCzIRLlRUb2tlbml6ZU9wdGlvbnMi"
"LAoLRUtleUNvbnRlbnQSBwoDS0VZEAESCAoERklMRRACEgoKBkNVU1RPTRADIhIKBFRSZWYSCgoC"
"aWQYASABKAciLgoIVEFydGljbGUSGAoDa2V5GAEgAygLMgsuVFNlYXJjaEtleSoICAIQgICAgAIi"
"/AEKC1RHenRPcHRpb25zEh8KCkRlZmF1bHRLZXkYASABKAsyCy5UU2VhcmNoS2V5EiAKElN0b3Jl"
"QXJ0aWNsZVRpdGxlcxgCIAEoCDoEdHJ1ZRIwCglXaWxkQ2FyZHMYAyABKAsyHS5UR3p0T3B0aW9u"
"cy5UV2lsZENhcmRPcHRpb25zEhcKCVByb3BhZ2F0ZRhkIAEoCDoEdHJ1ZRpfChBUV2lsZENhcmRP"
"cHRpb25zEhcKCUV4YWN0Rm9ybRgBIAEoCDoEdHJ1ZRIaCgxBcnRpY2xlU3Vic3QYAiABKAg6BHRy"
"dWUSFgoHQW55V29yZBgDIAEoCDoFZmFsc2UagB0KJmNvbnRyaWIvbGlicy9wcm90b2J1Zi9kZXNj"
"cmlwdG9yLnByb3RvEg9nb29nbGUucHJvdG9idWYiRwoRRmlsZURlc2NyaXB0b3JTZXQSMgoEZmls"
"ZRgBIAMoCzIkLmdvb2dsZS5wcm90b2J1Zi5GaWxlRGVzY3JpcHRvclByb3RvItwCChNGaWxlRGVz"
"Y3JpcHRvclByb3RvEgwKBG5hbWUYASABKAkSDwoHcGFja2FnZRgCIAEoCRISCgpkZXBlbmRlbmN5"
"GAMgAygJEjYKDG1lc3NhZ2VfdHlwZRgEIAMoCzIgLmdvb2dsZS5wcm90b2J1Zi5EZXNjcmlwdG9y"
"UHJvdG8SNwoJZW51bV90eXBlGAUgAygLMiQuZ29vZ2xlLnByb3RvYnVmLkVudW1EZXNjcmlwdG9y"
"UHJvdG8SOAoHc2VydmljZRgGIAMoCzInLmdvb2dsZS5wcm90b2J1Zi5TZXJ2aWNlRGVzY3JpcHRv"
"clByb3RvEjgKCWV4dGVuc2lvbhgHIAMoCzIlLmdvb2dsZS5wcm90b2J1Zi5GaWVsZERlc2NyaXB0"
"b3JQcm90bxItCgdvcHRpb25zGAggASgLMhwuZ29vZ2xlLnByb3RvYnVmLkZpbGVPcHRpb25zIqkD"
"Cg9EZXNjcmlwdG9yUHJvdG8SDAoEbmFtZRgBIAEoCRI0CgVmaWVsZBgCIAMoCzIlLmdvb2dsZS5w"
"cm90b2J1Zi5GaWVsZERlc2NyaXB0b3JQcm90bxI4CglleHRlbnNpb24YBiADKAsyJS5nb29nbGUu"
"cHJvdG9idWYuRmllbGREZXNjcmlwdG9yUHJvdG8SNQoLbmVzdGVkX3R5cGUYAyADKAsyIC5nb29n"
"bGUucHJvdG9idWYuRGVzY3JpcHRvclByb3RvEjcKCWVudW1fdHlwZRgEIAMoCzIkLmdvb2dsZS5w"
"cm90b2J1Zi5FbnVtRGVzY3JpcHRvclByb3RvEkgKD2V4dGVuc2lvbl9yYW5nZRgFIAMoCzIvLmdv"
"b2dsZS5wcm90b2J1Zi5EZXNjcmlwdG9yUHJvdG8uRXh0ZW5zaW9uUmFuZ2USMAoHb3B0aW9ucxgH"
"IAEoCzIfLmdvb2dsZS5wcm90b2J1Zi5NZXNzYWdlT3B0aW9ucxosCg5FeHRlbnNpb25SYW5nZRIN"
"CgVzdGFydBgBIAEoBRILCgNlbmQYAiABKAUilAUKFEZpZWxkRGVzY3JpcHRvclByb3RvEgwKBG5h"
"bWUYASABKAkSDgoGbnVtYmVyGAMgASgFEjoKBWxhYmVsGAQgASgOMisuZ29vZ2xlLnByb3RvYnVm"
"LkZpZWxkRGVzY3JpcHRvclByb3RvLkxhYmVsEjgKBHR5cGUYBSABKA4yKi5nb29nbGUucHJvdG9i"
"dWYuRmllbGREZXNjcmlwdG9yUHJvdG8uVHlwZRIRCgl0eXBlX25hbWUYBiABKAkSEAoIZXh0ZW5k"
"ZWUYAiABKAkSFQoNZGVmYXVsdF92YWx1ZRgHIAEoCRIuCgdvcHRpb25zGAggASgLMh0uZ29vZ2xl"
"LnByb3RvYnVmLkZpZWxkT3B0aW9ucyK2AgoEVHlwZRIPCgtUWVBFX0RPVUJMRRABEg4KClRZUEVf"
"RkxPQVQQAhIOCgpUWVBFX0lOVDY0EAMSDwoLVFlQRV9VSU5UNjQQBBIOCgpUWVBFX0lOVDMyEAUS"
"EAoMVFlQRV9GSVhFRDY0EAYSEAoMVFlQRV9GSVhFRDMyEAcSDQoJVFlQRV9CT09MEAgSDwoLVFlQ"
"RV9TVFJJTkcQCRIOCgpUWVBFX0dST1VQEAoSEAoMVFlQRV9NRVNTQUdFEAsSDgoKVFlQRV9CWVRF"
"UxAMEg8KC1RZUEVfVUlOVDMyEA0SDQoJVFlQRV9FTlVNEA4SEQoNVFlQRV9TRklYRUQzMhAPEhEK"
"DVRZUEVfU0ZJWEVENjQQEBIPCgtUWVBFX1NJTlQzMhAREg8KC1RZUEVfU0lOVDY0EBIiQwoFTGFi"
"ZWwSEgoOTEFCRUxfT1BUSU9OQUwQARISCg5MQUJFTF9SRVFVSVJFRBACEhIKDkxBQkVMX1JFUEVB"
"VEVEEAMijAEKE0VudW1EZXNjcmlwdG9yUHJvdG8SDAoEbmFtZRgBIAEoCRI4CgV2YWx1ZRgCIAMo"
"CzIpLmdvb2dsZS5wcm90b2J1Zi5FbnVtVmFsdWVEZXNjcmlwdG9yUHJvdG8SLQoHb3B0aW9ucxgD"
"IAEoCzIcLmdvb2dsZS5wcm90b2J1Zi5FbnVtT3B0aW9ucyJsChhFbnVtVmFsdWVEZXNjcmlwdG9y"
"UHJvdG8SDAoEbmFtZRgBIAEoCRIOCgZudW1iZXIYAiABKAUSMgoHb3B0aW9ucxgDIAEoCzIhLmdv"
"b2dsZS5wcm90b2J1Zi5FbnVtVmFsdWVPcHRpb25zIpABChZTZXJ2aWNlRGVzY3JpcHRvclByb3Rv"
"EgwKBG5hbWUYASABKAkSNgoGbWV0aG9kGAIgAygLMiYuZ29vZ2xlLnByb3RvYnVmLk1ldGhvZERl"
"c2NyaXB0b3JQcm90bxIwCgdvcHRpb25zGAMgASgLMh8uZ29vZ2xlLnByb3RvYnVmLlNlcnZpY2VP"
"cHRpb25zIn8KFU1ldGhvZERlc2NyaXB0b3JQcm90bxIMCgRuYW1lGAEgASgJEhIKCmlucHV0X3R5"
"cGUYAiABKAkSEwoLb3V0cHV0X3R5cGUYAyABKAkSLwoHb3B0aW9ucxgEIAEoCzIeLmdvb2dsZS5w"
"cm90b2J1Zi5NZXRob2RPcHRpb25zIqQDCgtGaWxlT3B0aW9ucxIUCgxqYXZhX3BhY2thZ2UYASAB"
"KAkSHAoUamF2YV9vdXRlcl9jbGFzc25hbWUYCCABKAkSIgoTamF2YV9tdWx0aXBsZV9maWxlcxgK"
"IAEoCDoFZmFsc2USRgoMb3B0aW1pemVfZm9yGAkgASgOMikuZ29vZ2xlLnByb3RvYnVmLkZpbGVP"
"cHRpb25zLk9wdGltaXplTW9kZToFU1BFRUQSIQoTY2NfZ2VuZXJpY19zZXJ2aWNlcxgQIAEoCDoE"
"dHJ1ZRIjChVqYXZhX2dlbmVyaWNfc2VydmljZXMYESABKAg6BHRydWUSIQoTcHlfZ2VuZXJpY19z"
"ZXJ2aWNlcxgSIAEoCDoEdHJ1ZRJDChR1bmludGVycHJldGVkX29wdGlvbhjnByADKAsyJC5nb29n"
"bGUucHJvdG9idWYuVW5pbnRlcnByZXRlZE9wdGlvbiI6CgxPcHRpbWl6ZU1vZGUSCQoFU1BFRUQQ"
"ARINCglDT0RFX1NJWkUQAhIQCgxMSVRFX1JVTlRJTUUQAyoJCOgHEICAgIACIrgBCg5NZXNzYWdl"
"T3B0aW9ucxImChdtZXNzYWdlX3NldF93aXJlX2Zvcm1hdBgBIAEoCDoFZmFsc2USLgofbm9fc3Rh"
"bmRhcmRfZGVzY3JpcHRvcl9hY2Nlc3NvchgCIAEoCDoFZmFsc2USQwoUdW5pbnRlcnByZXRlZF9v"
"cHRpb24Y5wcgAygLMiQuZ29vZ2xlLnByb3RvYnVmLlVuaW50ZXJwcmV0ZWRPcHRpb24qCQjoBxCA"
"gICAAiKUAgoMRmllbGRPcHRpb25zEjoKBWN0eXBlGAEgASgOMiMuZ29vZ2xlLnByb3RvYnVmLkZp"
"ZWxkT3B0aW9ucy5DVHlwZToGU1RSSU5HEg4KBnBhY2tlZBgCIAEoCBIZCgpkZXByZWNhdGVkGAMg"
"ASgIOgVmYWxzZRIcChRleHBlcmltZW50YWxfbWFwX2tleRgJIAEoCRJDChR1bmludGVycHJldGVk"
"X29wdGlvbhjnByADKAsyJC5nb29nbGUucHJvdG9idWYuVW5pbnRlcnByZXRlZE9wdGlvbiIvCgVD"
"VHlwZRIKCgZTVFJJTkcQABIICgRDT1JEEAESEAoMU1RSSU5HX1BJRUNFEAIqCQjoBxCAgICAAiJd"
"CgtFbnVtT3B0aW9ucxJDChR1bmludGVycHJldGVkX29wdGlvbhjnByADKAsyJC5nb29nbGUucHJv"
"dG9idWYuVW5pbnRlcnByZXRlZE9wdGlvbioJCOgHEICAgIACImIKEEVudW1WYWx1ZU9wdGlvbnMS"
"QwoUdW5pbnRlcnByZXRlZF9vcHRpb24Y5wcgAygLMiQuZ29vZ2xlLnByb3RvYnVmLlVuaW50ZXJw"
"cmV0ZWRPcHRpb24qCQjoBxCAgICAAiJgCg5TZXJ2aWNlT3B0aW9ucxJDChR1bmludGVycHJldGVk"
"X29wdGlvbhjnByADKAsyJC5nb29nbGUucHJvdG9idWYuVW5pbnRlcnByZXRlZE9wdGlvbioJCOgH"
"EICAgIACIl8KDU1ldGhvZE9wdGlvbnMSQwoUdW5pbnRlcnByZXRlZF9vcHRpb24Y5wcgAygLMiQu"
"Z29vZ2xlLnByb3RvYnVmLlVuaW50ZXJwcmV0ZWRPcHRpb24qCQjoBxCAgICAAiKeAgoTVW5pbnRl"
"cnByZXRlZE9wdGlvbhI7CgRuYW1lGAIgAygLMi0uZ29vZ2xlLnByb3RvYnVmLlVuaW50ZXJwcmV0"
"ZWRPcHRpb24uTmFtZVBhcnQSGAoQaWRlbnRpZmllcl92YWx1ZRgDIAEoCRIaChJwb3NpdGl2ZV9p"
"bnRfdmFsdWUYBCABKAQSGgoSbmVnYXRpdmVfaW50X3ZhbHVlGAUgASgDEhQKDGRvdWJsZV92YWx1"
"ZRgGIAEoARIUCgxzdHJpbmdfdmFsdWUYByABKAwSFwoPYWdncmVnYXRlX3ZhbHVlGAggASgJGjMK"
"CE5hbWVQYXJ0EhEKCW5hbWVfcGFydBgBIAIoCRIUCgxpc19leHRlbnNpb24YAiACKAhCKQoTY29t"
"Lmdvb2dsZS5wcm90b2J1ZkIQRGVzY3JpcHRvclByb3Rvc0gBGqYBCipxdWFsaXR5L2dsb2JhbF9z"
"ZWFyY2gvc3lubm9ybS9zeW5zZXQucHJvdG8SCE5TeW5Ob3JtGhlkaWN0L2dhemV0dGVlci9iYXNl"
"LnByb3RvIhcKB1RTeW5zZXQSDAoEUmVwchgBIAIoCSI6CgdUTWVtYmVyEhgKA2tleRgBIAMoCzIL"
"LlRTZWFyY2hLZXkSFQoGU3luc2V0GAIgAigLMgUuVFJlZhodEg0KC0FydGljbGVEYXRhGgwKClRp"
"dGxlSW5kZXgi3gEIAhAAGg8KDUV4YWN0Rm9ybVRyaWUiCwoJTGVtbWFUcmllKgwKClBocmFzZVRy"
"aWUyDgoMQ29tcG91bmRUcmllOnwKBgoEEgIAABIGCgQSAgAAGgYKBBICAAAiBgoEEgIAACoGCgQS"
"AgAAMgYKBBICAAA6GwoEEgIAABIBARoBASIBASoBATIBAToBAUIBAUoGCgQSAgAAUgYKBBICAADS"
"AwkKB0ZpbHRlcnPqAxAKDlNpbXBsZUFydGljbGVzQhIKEENvbXBvdW5kRWxlbWVudHNKCgoIV29y"
"ZFRyaWX6zt6tvu/wDQoAAAAhPeUjCwAAAEFydGljbGVEYXRhkgAAAA8Ziv8eEwoR0L3QvtCy0YvQ"
"uSDQtNC+0LwCczAfBxIFDQAAAAAAHwcSBQ0AAAAAAB4HCgVjYXRjaAJzMR8HEgUNLAAAAAAfBxIF"
"DSwAAAAAHgwKCmJlY29tZSBpbGwCczIfBxIFDUwAAAAAHwcSBQ1MAAAAAB4ICgbQutC+0YECczMf"
"BxIFDXEAAAAAHwcSBQ1xAAAAABAAAABDb21wb3VuZEVsZW1lbnRzBAAAADfcQ8gAAAAADAAAAENv"
"bXBvdW5kVHJpZQAAAADJuTZGDQAAAEV4YWN0Rm9ybVRyaWUAAAAAybk2RgcAAABGaWx0ZXJzFAAA"
"APeEdfEAAAAAAAAAAAIAAAAAAAAAAAAAAAkAAABMZW1tYVRyaWUAAAAAybk2RgoAAABQaHJhc2VU"
"cmllVQAAACOLjeVAAEAAQABJFQ45QABAAEAAgBkGAUkRFidAAEAAQABAJUAAQABAAIApCgGIDgUC"
"AUAFQABAAEAAgAkAAYASBAGJLgYSDAFAHUAAQABAAIAhCAGAMg4BDgAAAFNpbXBsZUFydGljbGVz"
"NAAAAAXSUGIIAAAAGAAAACIAAAA4AAAAQgAAAF0AAABnAAAAfgAAAIgAAAAAAAAAAgAAAAAAAAAA"
"AAAACgAAAFRpdGxlSW5kZXgVAAAAz11gzUAAQHNAAIkyBQxMiDEELIAwAIAzcQgAAABXb3JkVHJp"
"ZaoAAAAkDGKZSARTSToSHUAEQD5ABMBBLkAEgDAyQDRABEA+QASAPAlAPUAEQD5ABEAyQARISwhA"
"BIA5BUA+QARAQUAEQEJABEBAQARAPkAEQDlABEA6QASAMA5AAMljIkoVQABIbwxAAEBsQACAZClA"
"YUAAQHRAAEBjQACAaBNIYhhAAEBlQABAY0AAQG9AAEBtQACAZR3AYSVAAEB0QABAY0AAgGgZQGlA"
"AEBsQACAbCH6zt6tvu/wDQ==";

class TSynnormTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TSynnormTest);
        UNIT_TEST(BasicTest)
        UNIT_TEST(NoSortTest)
        UNIT_TEST(CollocationToSynsetTest)
        UNIT_TEST(UseMorphologyAtLastStateTest)
        UNIT_TEST(AmbigiousLemmatizationTest)
        UNIT_TEST(SeveralLemmasFromOneSynset)
        UNIT_TEST(DeleteStopWordsTest)
        UNIT_TEST(NormalizeWildCardsTest)
    UNIT_TEST_SUITE_END();

public:
    TSynnormTest() {
        TTempFile temp("tmp_synnorm");
        {
            TFixedBufferFileOutput out(temp.Name());
            out << Base64Decode(data);
        }
        Normalizer_.LoadSynsets(temp.Name());
    }

private:
    void BasicTest();
    void NoSortTest();
    void CollocationToSynsetTest();
    void UseMorphologyAtLastStateTest();
    void AmbigiousLemmatizationTest();
    void SeveralLemmasFromOneSynset();
    void DeleteStopWordsTest();
    void NormalizeWildCardsTest();

private:
    TSynNormalizer Normalizer_;
};

UNIT_TEST_SUITE_REGISTRATION(TSynnormTest);

void TSynnormTest::BasicTest() {
    UNIT_ASSERT_STRINGS_EQUAL(Normalizer_.NormalizeUTF8("новостройка"), "новый дом");
}

void TSynnormTest::NoSortTest() {
    UNIT_ASSERT_STRINGS_EQUAL(Normalizer_.NormalizeUTF8("новый дом два"), "два новый дом");
    UNIT_ASSERT_STRINGS_EQUAL(Normalizer_.NormalizeUTF8("новый дом два", TLangMask(LANG_ENG, LANG_RUS), false), "новый дом два");
}

void TSynnormTest::CollocationToSynsetTest() {
    UNIT_ASSERT_STRINGS_EQUAL(Normalizer_.NormalizeUTF8("c atch a cold"), "catch cold");
    UNIT_ASSERT_STRINGS_EQUAL(Normalizer_.NormalizeUTF8("раз новый дом"), "новый дом раз");
    UNIT_ASSERT_STRINGS_EQUAL(Normalizer_.NormalizeUTF8("новый дом два"), "два новый дом");
}

void TSynnormTest::UseMorphologyAtLastStateTest() {
    UNIT_ASSERT_STRINGS_EQUAL(Normalizer_.NormalizeUTF8("caught a cold"), "catch cold");
}

void TSynnormTest::AmbigiousLemmatizationTest() {
    UNIT_ASSERT_STRINGS_EQUAL(Normalizer_.NormalizeUTF8("лука седла"), "лука седло");
}

void TSynnormTest::SeveralLemmasFromOneSynset() {
    UNIT_ASSERT_STRINGS_EQUAL(Normalizer_.NormalizeUTF8("косу"), "кос");
}

void TSynnormTest::DeleteStopWordsTest() {
    UNIT_ASSERT_STRINGS_EQUAL(Normalizer_.NormalizeUTF8("новый это дом"), "дом новый");
    UNIT_ASSERT_STRINGS_EQUAL(Normalizer_.NormalizeUTF8("новый скачать дом"), "дом новый");
}

void TSynnormTest::NormalizeWildCardsTest() {
    const TString normalized = Normalizer_.NormalizeUTF8("zeus dethmaul");
    const TString normalizedNoSort = Normalizer_.NormalizeUTF8("zeus dethmaul", TLangMask(LANG_ENG, LANG_RUS), false);
    UNIT_ASSERT_STRINGS_EQUAL(normalized, "dethmaul zeus");
    UNIT_ASSERT_STRINGS_EQUAL(NSynNorm::NormalizeWildcardsUsingSynnorm(normalized), "dethm zeus");
    UNIT_ASSERT_STRINGS_EQUAL(normalizedNoSort, "zeus dethmaul");
    UNIT_ASSERT_STRINGS_EQUAL(NSynNorm::NormalizeWildcardsUsingSynnorm(normalizedNoSort), "zeus dethm");
}
