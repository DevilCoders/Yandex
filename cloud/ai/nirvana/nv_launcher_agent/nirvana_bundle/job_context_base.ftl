[#function job_context_base]
  [#local full_meta = {
      "workflowUid": meta.workflow_uid,
      "workflowInstanceUid": meta.workflow_instance_uid,
      "workflowURL": meta.workflow_url,
      "operationUid": meta.operation_uid,
      "blockUid": meta.block_uid,
      "blockCode": meta.block_code,
      "blockURL": meta.block_url,
      "processUid": meta.process_uid,
      "processURL": meta.process_url,
      "priority": {
        "min": meta.min_priority,
        "max": meta.max_priority,
        "value": meta.priority,
        "normValue": meta.norm_priority
      },
      "description": meta.description,
      "owner": meta.owner,
      "quotaProject": meta.quota_project
  }]
  [#if meta.parent_block_uid??][#local full_meta = full_meta + { "parentBlockUid": meta.parent_block_uid }][/#if]
  [#if meta.parent_block_code??][#local full_meta = full_meta + { "parentBlockCode": meta.parent_block_code }][/#if]
  [#if meta.parent_block_url??][#local full_meta = full_meta + { "parentBlockURL": meta.parent_block_url }][/#if]
  [#return {
    "meta": full_meta,
    "parameters": map(param, param_value),
    "inputs": fileset_paths(input),
    "outputs": fileset_paths(output),
    "ports": {
      "udp": port_items(is_udp_port),
      "tcp": port_items(is_tcp_port)
    },
    "inputItems": fileset_items(input),
    "outputItems": fileset_items(output),
    "status": {
      "errorMsg": status.error_msg,
      "successMsg": status.success_msg,
      "log": status.log
    }
  }]
[/#function]

[#function param_value value]
  [#if value?is_enumerable][#return map(as_list(value), param_value)]
  [#elseif value?is_string][#return as_string(value)]
  [#else][#return value][/#if]
[/#function]

[#function fileset_paths filesets][#return map(filter(filesets, is_not_empty), as_list)][/#function]

[#function fileset_items filesets][#return map(filter(filesets, is_not_empty), file_items)][/#function]
[#function file_items fileset][#return map(as_list(fileset), file_item)][/#function]
[#function file_item file]
  [#local item = {
    "dataType": file.data_type,
    "wasUnpacked": file.was_unpacked,
    "unpackedDir": file.unpacked
  }]
  [#attempt][#local item = item + { "unpackedFile": file.unpacked_single }][#recover][/#attempt]
  [#if file.download_url??][#local item = item + { "downloadURL": file.download_url }][/#if]
  [#if file.link_name??][#local item = item + { "linkName": file.link_name }][/#if]
  [#return item]
[/#function]

[#function port_items port_filter][#return map(filter(port, port_filter), port_number)][/#function]
[#function is_udp_port port][#return port.protocol == "UDP"][/#function]
[#function is_tcp_port port][#return port.protocol == "TCP"][/#function]
[#function port_number port][#return port.number][/#function]

[#function is_not_empty arg][#return arg?size > 0][/#function]
[#function as_list arg][#return [] + arg][/#function]
[#function as_string arg][#return "" + arg][/#function]
[#function map src mapper]
  [#if src?is_hash_ex]
    [#local result = {}]
    [#list src?keys as key][#local result = result + { key: mapper(src[key]) }][/#list]
    [#return result]
  [#elseif src?is_enumerable]
    [#local result = []]
    [#list src as item][#local result = result + [mapper(item)]][/#list]
    [#return result]
  [/#if]
[/#function]
[#function filter src predicate]
  [#if src?is_hash_ex]
    [#local result = {}]
    [#list src?keys as key]
      [#local value = src[key]]
      [#if predicate(value)][#local result = result + { key: value }][/#if]
    [/#list]
    [#return result]
  [#elseif src?is_enumerable]
    [#local result = []]
    [#list src as item]
      [#if predicate(item)][#local result = result + [item]][/#if]
    [/#list]
    [#return result]
  [/#if]
[/#function]
