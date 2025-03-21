---
editable: false
---

# Method list
Retrieves the list of Registry resources in the specified folder.
 

 
## HTTP request {#https-request}
```
GET https://container-registry.{{ api-host }}/container-registry/v1/registries
```
 
## Query parameters {#query_params}
 
Parameter | Description
--- | ---
folderId | <p>Required. ID of the folder to list registries in.</p> <p>To get the folder ID use a <a href="/docs/resource-manager/api-ref/Folder/list">list</a> request.</p> <p>The maximum string length in characters is 50.</p> 
pageSize | <p>The maximum number of results per page to return. If the number of available results is larger than <a href="/docs/container-registry/api-ref/Registry/list#query_params">pageSize</a>, the service returns a <a href="/docs/container-registry/api-ref/Registry/list#responses">nextPageToken</a> that can be used to get the next page of results in subsequent list requests. Default value: 100.</p> <p>The maximum value is 1000.</p> 
pageToken | <p>Page token. To get the next page of results, set <a href="/docs/container-registry/api-ref/Registry/list#query_params">pageToken</a> to the <a href="/docs/container-registry/api-ref/Registry/list#responses">nextPageToken</a> returned by a previous list request.</p> <p>The maximum string length in characters is 100.</p> 
filter | <p>A filter expression that filters resources listed in the response. The expression must specify:</p> <ol> <li>The field name. Currently you can use filtering only on <a href="/docs/container-registry/api-ref/Registry#representation">Registry.name</a> field.</li> <li>An ``=`` operator.</li> <li>The value in double quotes (``"``). Must be 3-63 characters long and match the regular expression ``[a-z][-a-z0-9]{1,61}[a-z0-9]``.</li> </ol> <p>The maximum string length in characters is 1000.</p> 
 
## Response {#responses}
**HTTP Code: 200 - OK**

```json 
{
  "registries": [
    {
      "id": "string",
      "folderId": "string",
      "name": "string",
      "status": "string",
      "createdAt": "string",
      "labels": "object"
    }
  ],
  "nextPageToken": "string"
}
```

 
Field | Description
--- | ---
registries[] | **object**<br><p>List of Registry resources.</p> 
registries[].<br>id | **string**<br><p>Output only. ID of the registry.</p> 
registries[].<br>folderId | **string**<br><p>ID of the folder that the registry belongs to.</p> 
registries[].<br>name | **string**<br><p>Name of the registry.</p> 
registries[].<br>status | **string**<br><p>Output only. Status of the registry.</p> <ul> <li>CREATING: Registry is being created.</li> <li>ACTIVE: Registry is ready to use.</li> <li>DELETING: Registry is being deleted.</li> </ul> 
registries[].<br>createdAt | **string** (date-time)<br><p>Output only. Creation timestamp in <a href="https://www.ietf.org/rfc/rfc3339.txt">RFC3339</a> text format.</p> <p>String in <a href="https://www.ietf.org/rfc/rfc3339.txt">RFC3339</a> text format.</p> 
registries[].<br>labels | **object**<br><p>Resource labels as ``key:value`` pairs. Maximum of 64 per resource.</p> 
nextPageToken | **string**<br><p>This token allows you to get the next page of results for list requests. If the number of results is larger than <a href="/docs/container-registry/api-ref/Registry/list#query_params">pageSize</a>, use the <a href="/docs/container-registry/api-ref/Registry/list#responses">nextPageToken</a> as the value for the <a href="/docs/container-registry/api-ref/Registry/list#query_params">pageToken</a> query parameter in the next list request. Each subsequent list request will have its own <a href="/docs/container-registry/api-ref/Registry/list#responses">nextPageToken</a> to continue paging through the results.</p> 