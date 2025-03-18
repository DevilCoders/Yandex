$capture = Re2::Capture("\\w+\\-\\w{1,2}\\-(?P<page_id>\\d+)\-(?P<block_id>\\d+)");

$touch_blocks = select Advertising from hahn.`home/webmaster/prod/export/turbo/turbo-hosts`;

$get_ads = Python::get_ads(Callable<(String?)->List<String>>,
@@
import json

def get_ads(obj):
  obj = json.loads(obj)
  result = []
  for item in obj:
    if item['type'] == 'Yandex':
      result.append(item['id'])
  return result
@@);

$ads = SELECT
    $get_ads(Yson::ConvertToString(Advertising)) as touch,
from $touch_blocks;

$blocks = (
    select $capture(touch) as block, 'touch' as site_version from $ads
    flatten by touch
);

SELECT cast(block.page_id as uint64) as page_id, cast(block.block_id as uint64) as block_id, site_version
FROM $blocks
UNION ALL
select page_id, block_id, site_version
from hahn.`home/comdep-analytics/chaos-ad/pi/v2/dicts_joined/latest/blocks`
where site_version = 'turbo_desktop';
