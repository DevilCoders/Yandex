---
editable: false
---

# Method list
Retrieves the list of SQL Server backups available for the specified folder.
 

 
## HTTP request {#https-request}
```
GET https://mdb.{{ api-host }}/mdb/sqlserver/v1/backups
```
 
## Query parameters {#query_params}
 
Parameter | Description
--- | ---
folderId | <p>Required. ID of the folder to list backups in.</p> <p>To get the folder ID, use a <a href="/docs/resource-manager/api-ref/Folder/list">list</a> request.</p> <p>The maximum string length in characters is 50.</p> 
pageSize | <p>The maximum number of results per page to return.</p> <p>If the number of available results is larger than <a href="/docs/managed-sqlserver/api-ref/Backup/list#query_params">pageSize</a>, the service returns a <a href="/docs/managed-sqlserver/api-ref/Backup/list#responses">nextPageToken</a> that can be used to get the next page of results in subsequent list requests.</p> <p>The maximum value is 1000.</p> 
pageToken | <p>Page token. To get the next page of results, set <a href="/docs/managed-sqlserver/api-ref/Backup/list#query_params">pageToken</a> to the <a href="/docs/managed-sqlserver/api-ref/Backup/list#responses">nextPageToken</a> returned by the previous list request.</p> <p>The maximum string length in characters is 100.</p> 
 
## Response {#responses}
**HTTP Code: 200 - OK**

```json 
{
  "backups": [
    {
      "id": "string",
      "folderId": "string",
      "createdAt": "string",
      "sourceClusterId": "string",
      "startedAt": "string",
      "databases": [
        "string"
      ]
    }
  ],
  "nextPageToken": "string"
}
```

 
Field | Description
--- | ---
backups[] | **object**<br><p>List of SQL Server backups.</p> 
backups[].<br>id | **string**<br><p>ID of the backup.</p> 
backups[].<br>folderId | **string**<br><p>ID of the folder that the backup belongs to.</p> 
backups[].<br>createdAt | **string** (date-time)<br><p>Time when the backup operation was completed.</p> <p>String in <a href="https://www.ietf.org/rfc/rfc3339.txt">RFC3339</a> text format.</p> 
backups[].<br>sourceClusterId | **string**<br><p>ID of the SQL Server cluster that the backup was created for.</p> 
backups[].<br>startedAt | **string** (date-time)<br><p>Time when the backup operation was started.</p> <p>String in <a href="https://www.ietf.org/rfc/rfc3339.txt">RFC3339</a> text format.</p> 
backups[].<br>databases[] | **string**<br><p>List of databases included in the backup.</p> 
nextPageToken | **string**<br><p>This token allows you to get the next page of results for ListBackups requests.</p> <p>If the number of results is larger than <a href="/docs/managed-sqlserver/api-ref/Backup/list#query_params">pageSize</a>, use the <a href="/docs/managed-sqlserver/api-ref/Backup/list#responses">nextPageToken</a> as the value for the <a href="/docs/managed-sqlserver/api-ref/Backup/list#query_params">pageToken</a> parameter in the next ListBackups request.</p> <p>Each subsequent ListBackups request has its own <a href="/docs/managed-sqlserver/api-ref/Backup/list#responses">nextPageToken</a> to continue paging through the results.</p> <p>The maximum string length in characters is 100.</p> 