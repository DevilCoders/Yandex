---
editable: false
---

# Image
A set of methods for managing Image resources.
## JSON Representation {#representation}
```json 
{
  "id": "string",
  "folderId": "string",
  "createdAt": "string",
  "name": "string",
  "description": "string",
  "labels": "object",
  "family": "string",
  "storageSize": "string",
  "minDiskSize": "string",
  "productIds": [
    "string"
  ],
  "status": "string",
  "os": {
    "type": "string"
  },
  "pooled": true
}
```
 
Field | Description
--- | ---
id | **string**<br><p>ID of the image.</p> 
folderId | **string**<br><p>ID of the folder that the image belongs to.</p> 
createdAt | **string** (date-time)<br><p>String in <a href="https://www.ietf.org/rfc/rfc3339.txt">RFC3339</a> text format.</p> 
name | **string**<br><p>Name of the image. 1-63 characters long.</p> 
description | **string**<br><p>Description of the image. 0-256 characters long.</p> 
labels | **object**<br><p>Resource labels as ``key:value`` pairs. Maximum of 64 per resource.</p> 
family | **string**<br><p>The name of the image family to which this image belongs.</p> <p>You can get the most recent image from a family by using the <a href="/docs/compute/api-ref/Image/getLatestByFamily">getLatestByFamily</a> request and create the disk from this image.</p> 
storageSize | **string** (int64)<br><p>The size of the image, specified in bytes.</p> 
minDiskSize | **string** (int64)<br><p>Minimum size of the disk which will be created from this image.</p> 
productIds[] | **string**<br><p>License IDs that indicate which licenses are attached to this resource. License IDs are used to calculate additional charges for the use of the virtual machine.</p> <p>The correct license ID is generated by the platform. IDs are inherited by new resources created from this resource.</p> <p>If you know the license IDs, specify them when you create the image. For example, if you create a disk image using a third-party utility and load it into Object Storage, the license IDs will be lost. You can specify them in the <a href="/docs/compute/api-ref/Image/create">create</a> request.</p> 
status | **string**<br><p>Current status of the image.</p> <ul> <li>CREATING: Image is being created.</li> <li>READY: Image is ready to use.</li> <li>ERROR: Image encountered a problem and cannot operate.</li> <li>DELETING: Image is being deleted.</li> </ul> 
os | **object**<br><p>Operating system that is contained in the image.</p> 
os.<br>type | **string**<br><p>Operating system type. The default is ``LINUX``.</p> <p>This field is used to correctly emulate a vCPU and calculate the cost of using an instance.</p> <ul> <li>LINUX: Linux operating system.</li> <li>WINDOWS: Windows operating system.</li> </ul> 
pooled | **boolean** (boolean)<br><p>When true, indicates there is an image pool for fast creation disks from the image.</p> 

## Methods {#methods}
Method | Description
--- | ---
[create](create.md) | Creates an image in the specified folder.
[delete](delete.md) | Deletes the specified image.
[get](get.md) | Returns the specified Image resource.
[getLatestByFamily](getLatestByFamily.md) | Returns the latest image that is part of an image family.
[list](list.md) | Retrieves the list of Image resources in the specified folder.
[listOperations](listOperations.md) | Lists operations for the specified image.
[update](update.md) | Updates the specified image.