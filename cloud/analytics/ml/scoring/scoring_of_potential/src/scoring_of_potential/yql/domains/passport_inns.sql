USE hahn;
PRAGMA yt.Pool = 'cloud_analytics_pool';

$result_table = '//home/cloud_analytics/scoring_of_potential/clean_id_graph/passport_inns';

$nodeId2Struct = ($id) -> {
    $node_list = String::SplitToList($id, '::');
    RETURN AsStruct($node_list[0] AS type, $node_list[1] AS value)
};

$neighbours = (
    SELECT * 
    FROM `//home/cloud_analytics/scoring_of_potential/clean_id_graph/neighbours`
    WHERE $nodeId2Struct(e0_src).type='passport_id'
);


$l0 = (
    SELECT
        DISTINCT 
            $nodeId2Struct(e0_src) AS src,
            $nodeId2Struct(e0_dst) AS dst,
            [e0_src, e0_dst] AS path,
            0 AS level
    FROM $neighbours
);

$l1 = (
    SELECT
        DISTINCT 
            $nodeId2Struct(e0_src) AS src,
            $nodeId2Struct(e1_dst) AS dst,
            [e0_src, e0_dst, e1_dst] AS path,
            1 AS level
    FROM $neighbours
    
);


$l2 = (
    SELECT
        DISTINCT 
            $nodeId2Struct(e0_src) AS src,
            $nodeId2Struct(e2_dst) AS dst,
            [e0_src, e0_dst, e1_dst, e2_dst] AS path,
            2 AS level
    FROM $neighbours
);



$l3 = (
    SELECT
        DISTINCT 
            $nodeId2Struct(e0_src) AS src,
            $nodeId2Struct(e3_dst) AS dst,
            [e0_src, e0_dst, e1_dst, e2_dst, e3_dst] AS path,
            3 AS level
    FROM $neighbours
);

$passport_inns = (
    SELECT src, 
           dst, 
           AGGREGATE_LIST(path) AS paths,
           COUNT(path) AS paths_count,
           MAX(level) AS max_level,
           MIN(level) AS min_level
    FROM (
        SELECT * FROM $l0 
        UNION ALL
        SELECT * FROM $l1
        UNION ALL
        SELECT * FROM $l2
        UNION ALL
        SELECT * FROM $l3
    )
    WHERE (dst.type = 'inn') AND (ListLength(path) = ListLength(ListUniq(path)))
    GROUP BY src, dst
);


INSERT INTO $result_table WITH TRUNCATE
SELECT * 
FROM $passport_inns