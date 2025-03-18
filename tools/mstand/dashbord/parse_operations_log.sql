use hahn;
pragma yt.Pool = 'mstand-dashboard';

$get_reduce_tot_cpu_ms = ($data) -> (
    Yson::YPathInt64($data.job_statistics, '/time/exec/$/completed/sorted_reduce/sum')
);

$get_observation_id = ($data) -> {
    $obs_id = Yson::YPath($data.pool, "/observations/0/observation_id");
    return if(not Yson::IsEntity($obs_id), Yson::ConvertToString($obs_id));
};

$ms_per_core = 24 * 60 * 60 * 1000;

$check_web_services = ($services) -> (
    DictLength(SetDifference(ToSet($services), {"web", "touch", "web-desktop-extended", "web-touch-extended", "web-auto", "web-auto-extended"})) == 0
);

$enable_cache = ($cmd) -> ("--enable-cache" in $cmd);

$use_cache = ($input_tables) -> (
    ListHasItems(
        ListFilter(
            $input_tables,
            ($x) -> ($x like '%squeeze/testids/yuid-reqid-testid-filter/0/%')
        )
    )
);

$structured_log = (
    select *
    from `home/mstand/logs/operations`
    with schema Struct<
        run_id:String,
        ts:UInt64,
        ts_msk:String,
        rectype:String,
        cmd:List<String>,
        data:Struct<
            services:List<String>?,
            yt_pool:String?,
            nirvana_data:Dict<String,String>?,
            pool:Yson?,
            operation_sid:String?,
            input_tables:List<String>?,
            experiments:List<String>?,
            operation_url:String?,
            operation_id:String?,
            job_statistics:Yson?,
        >?
    >
);

$start_runs = (
    select *
    from $structured_log
    where rectype = 'start'
);

$finish_runs = (
    select *
    from $structured_log
    where rectype = 'finish'
);

$run_info = (
    select
        start.run_id as run_id,
        finish.run_id is null as run_fail,
        if(finish.ts is null, 0, finish.ts - start.ts) as duration,
        SUBSTRING(start.ts_msk, 0, 10) as run_day,
        String::JoinFromList(ListSort(start.data.services), " ") as services,
        $check_web_services(start.data.services) as run_has_web,
        start.data.yt_pool as yt_pool,
        $enable_cache(start.cmd) as enable_cache,
        start.data.nirvana_data is not null as run_from_nirvana,
        $get_observation_id(start.data) as observation_id,
    from $start_runs as start
    left join $finish_runs as finish
    using (run_id)
);

$capture = Re2::Capture("^<(?P<testid>.*), (?P<service>.*)>$");
$get_testids = ($experiments) -> (
    ListSort(ListUniq(ListMap($experiments, ($x) -> ($capture($x).testid))))
);
$get_services = ($experiments) -> (
    ListSort(ListUniq(ListMap($experiments, ($x) -> ($capture($x).service))))
);

$make_operation_url = ($url) -> (
    AsTagged(
        AsStruct(
            $url as href,
            "link" as text
        ),
        "url"
    )
);

$start_operations = (
    select *
    from $structured_log
    where rectype = 'operation-start'
);

$finish_operations = (
    select *
    from $structured_log
    where rectype = 'operation-finish'
);

$yt_finish_operations = (
    select *
    from $structured_log
    where rectype like '%reduce-finish'
);

$operations_info = (
    select
        SUBSTRING(op_start.ts_msk, 0, 10) as run_operation_day,
        op_start.run_id as run_id,
        op_finish.ts - op_start.ts as duration,
        op_start.data.operation_sid as operation_sid,
        op_finish.data is null as operation_fail,
        $get_reduce_tot_cpu_ms(yt_op_finish.data) as cpu_tot,
        Math::Round(1.0 * $get_reduce_tot_cpu_ms(yt_op_finish.data) / $ms_per_core, -2) as cores,
        $use_cache(op_start.data.input_tables) as use_cache,
        $get_testids(op_start.data.experiments) as testids,
        String::JoinFromList($get_services(op_start.data.experiments), " ") as services,
        $make_operation_url(yt_op_finish.data.operation_url) as operation_url,
        yt_op_finish.data.operation_id as operation_id,
        $check_web_services($get_services(op_start.data.experiments)) as operation_has_web,
    from $start_operations as op_start
    left join $finish_operations as op_finish on op_finish.data.operation_sid = op_start.data.operation_sid
    join $yt_finish_operations as yt_op_finish on yt_op_finish.data.operation_sid = op_start.data.operation_sid
);

$yt_operation_log = (
    select
        operation_id,
        operation_type,
        pool,
        time_exec,
        1.0 * time_exec / $ms_per_core as cores,
    from range(`statbox/cube/daily/report_data/tech_report/yt_resource_consumption/v3/1d`, "2021-02-01") as yt_log
    group by operation_id, operation_type, (pool ?? "staff:" || authenticated_user) as pool, time_exec
);

$stream = (
    select
        ri.run_id as run_id,
        ri.duration as run_duration,
        ri.services as run_services,
        ri.yt_pool ?? yol.pool as yt_pool,
        oi.duration as op_duration,
        yol.time_exec ?? oi.cpu_tot as cpu_tot,
        yol.cores ?? oi.cores as cores,
        ri.*,
        oi.*,
    without ri.run_id, ri.duration, ri.services, ri.yt_pool, oi.run_id, oi.duration, oi.cpu_tot, oi.cores
    from $run_info as ri
    join $operations_info as oi on oi.run_id = ri.run_id
    left join $yt_operation_log as yol on yol.operation_id = oi.operation_id
);

insert into `home/mstand/logs/parsed_operations` with truncate
select * from $stream
order by run_day, run_id, operation_sid, run_operation_day;
