---
editable: false
---

# Method listCertificates
Retrieves the list of broker certificates for the specified broker.
 

 
## HTTP request {#https-request}
```
GET https://iot-broker.{{ api-host }}/iot-broker/v1/brokers/{brokerId}/certificates
```
 
## Path parameters {#path_params}
 
Parameter | Description
--- | ---
brokerId | <p>Required. ID of the broker to list certificates for.</p> <p>The maximum string length in characters is 50.</p> 
 
## Response {#responses}
**HTTP Code: 200 - OK**

```json 
{
  "certificates": [
    {
      "brokerId": "string",
      "fingerprint": "string",
      "certificateData": "string",
      "createdAt": "string"
    }
  ]
}
```

 
Field | Description
--- | ---
certificates[] | **object**<br><p>List of certificates for the specified broker.</p> 
certificates[].<br>brokerId | **string**<br><p>ID of the broker that the certificate belongs to.</p> 
certificates[].<br>fingerprint | **string**<br><p>SHA256 hash of the certificates.</p> 
certificates[].<br>certificateData | **string**<br><p>Public part of the certificate.</p> 
certificates[].<br>createdAt | **string** (date-time)<br><p>Creation timestamp.</p> <p>String in <a href="https://www.ietf.org/rfc/rfc3339.txt">RFC3339</a> text format.</p> 