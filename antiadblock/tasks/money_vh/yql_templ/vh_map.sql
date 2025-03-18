-- {{ file }} (file this query was created with)
$getString = ($json, $fieldPath) -> {return Yson::ConvertToString(Yson::YPath(Yson::ParseJson($json), $fieldPath));};
$getInt = ($json, $fieldPath) -> {return Yson::ConvertToInt64(Yson::YPath(Yson::ParseJson($json), $fieldPath));};
$isInt = ($json, $fieldPath) -> {return Yson::IsInt64(Yson::YPath(Yson::ParseJson($json), $fieldPath));};
$page_imp_video = (select * from `home/yabs/dict/PageImp` where AdTypeVideo == true);
$page = (select PageID, if(OptionsApp == true and OptionsMobile == true, 'app', 'web') as is_app from `home/yabs/dict/Page` where TargetType = 3);

$dspid = (
select
  DSPID,
  if(Title like '%-12' or Title like '%IMHO%', 'imho',
     if(Title like '%Audio%', 'audio',
        if(Title like '%-10' or Title like '%Auction%', 'auction',
           if(Tag = 'awaps', 'price', 'direct')))) as dsp_name
from `home/yabs/dict/DSPTemplate`
where (Tag == 'awaps' and DSPType in (2,6,7)) or DSPID == 1
union all
select 0 as DSPID, 'dsp_for_hits' as dsp_name
);

$BlockSettings_standart = (
select
  BlockSettingsID,
  case
    when $getInt(Settings, '/video-placement')==1 then 'instream'
    when $getInt(Settings, '/video-placement')==5 then 'interstitial'
    when $getInt(Settings, '/video-placement')==3 then 'inpage'
    else 'other'
  end as type,
  if($getString(Settings, '/video-category-name') like 'VH:%', 'vh', 'not_vh') as is_vh,
  if($getString(Settings, '/video-category-name') like 'VH:%', $getString(Settings, '/video-category-name'), 'other') as vh_category_name,
  $getInt(Settings, '/video-category-id') as vh_category_id,
  if($getInt(Settings, '/video-placement')==1 and $getInt(Settings, '/video-startdelay')==-2 and $getInt(Settings, '/video-linearity')==1, 'postroll', 'not_postroll') as is_postroll
from `home/yabs/dict/BlockSettings`
where Settings != "" and $isInt(Settings, '/video-placement')
);

$pddi =(
select PageID, ImpID, type, is_vh, vh_category_name, vh_category_id, is_postroll
from `home/yabs/dict/PageDSP` as pd
inner join $dspid as di on pd.DSPID == di.DSPID
inner join $BlockSettings_standart as bs on pd.BlockSettingsID == bs.BlockSettingsID
where pd.DSPID not in (5, 10) and di.dsp_name != 'imho'
group by
  pd.PageID as PageID,
  pd.ImpID as ImpID,
  bs.type as type,
  bs.is_vh as is_vh,
  bs.vh_category_name as vh_category_name,
  bs.vh_category_id as vh_category_id,
  bs.is_postroll as is_postroll
);

$result_path = "{{ vh_map_path }}" || cast(CurrentUtcDate() as String);
INSERT INTO $result_path
select
  piv.PageID as PageID,
  piv.ImpID as ImpID,
  pddi.type as type,
  pddi.is_vh as is_vh,
  pddi.vh_category_name as vh_category_name,
  pddi.vh_category_id as vh_category_id,
  pddi.is_postroll as is_postroll,
  page.is_app as is_app
from $page_imp_video as piv
left join $pddi as pddi on piv.PageID = pddi.PageID and piv.ImpID = pddi.ImpID
inner join $page as page on piv.PageID = page.PageID;
COMMIT;
