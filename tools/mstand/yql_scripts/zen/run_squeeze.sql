pragma AnsiInForEmptyOrNullableItemsCollections;

pragma library('squeeze_lib.sql');
import squeeze_lib symbols $squeeze;

declare $cluster as String;
declare $day as Date;
declare $yt_pool as String;
declare $transaction_id as String;
declare $experiments as List<Struct<testid:String, tmp_table_path:String>>;

use yt:$cluster;
pragma yt.Pool = $yt_pool;
pragma yt.ExternalTx = $transaction_id;

$testids = select aggregate_list(testid) from as_table($experiments);

evaluate for $exp in $experiments do begin
    $dst_path = $exp.tmp_table_path;
    insert into $dst_path with truncate
    select
        t.*,
        $exp.testid as testid,
        ListHas(applied_testids, $exp.testid) as is_match,
    without t.testids, t.applied_testids
    from $squeeze($day, $cluster, $testids) as t
    where $exp.testid in testids
    assume order by yuid, ts, action_index;
end do;
