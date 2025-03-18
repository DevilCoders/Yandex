-- {{ file }} (file this query was created with)
$service_id_to_pageid_names = (
  SELECT column0.0 as service_id, column0.1 as names FROM
  (SELECT Yson::ConvertTo(Yson::ParseJson('{{ service_id_to_pagename }}'), ParseType('Dict<String, List<String>>')) ?? DictCreate(ParseType('String'), ParseType('List<String>')))
  FLATTEN DICT BY column0
);
$service_id_to_pageid_names_table = (SELECT service_id, name FROM $service_id_to_pageid_names FLATTEN BY names as name);
$service_id_to_custom_pageid_names_flatten_by_service_id = (
  SELECT column0.0 as service_id, column0.1 as custom_pageid_names FROM
  (SELECT Yson::ConvertTo(Yson::ParseJson('{{ service_id_to_custom_pagename }}'), ParseType('Dict<String, Dict<String, List<Int64>>>'))
   ?? DictCreate(ParseType('String'), ParseType('Dict<String, List<Int64>>')))
  FLATTEN DICT BY column0
);
$service_id_to_custom_pageid_names = (
  SELECT service_id, cn.0 as name, cn.1 as pageids
  FROM $service_id_to_custom_pageid_names_flatten_by_service_id
  FLATTEN BY custom_pageid_names as cn
);

INSERT INTO `{{ partners_pageids_table }}` WITH TRUNCATE
SELECT
  s.service_id as service_id,
  s.name as name,
  cast(p.PageID as String) as pageid
FROM
  $service_id_to_pageid_names_table as s join `home/yabs/dict/Page` as p on s.name = p.Name
UNION ALL
SELECT
  service_id, name, cast(pageid as String) as pageid
FROM
  $service_id_to_custom_pageid_names
FLATTEN BY pageids as pageid
