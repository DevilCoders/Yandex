USE hahn;


$result_table = '{result_table_path}';
$neighbours_path = '{neighbours_path}';

$nodeId2Struct = ($id) -> {{
    $node_list = String::SplitToList($id, '::');
    RETURN AsStruct($node_list[0] AS type, $node_list[1] AS value)
}};

$neighbours = (
    SELECT * 
    FROM $neighbours_path
    WHERE $nodeId2Struct(e0_src).type='passport_id'
);


$l0 = (
    SELECT
        DISTINCT 
            e0_src AS src,
            e0_dst AS dst,
            ListNotNull([e0_src, e0_dst]) AS path
    FROM $neighbours
);

$l1 = (
    SELECT
        DISTINCT 
            e0_src AS src,
            e1_dst AS dst,
            ListNotNull([e0_src, e0_dst, e1_dst]) AS path
    FROM $neighbours
    
);


$l2 = (
    SELECT
        DISTINCT 
            e0_src AS src,
            e2_dst AS dst,
            ListNotNull([e0_src, e0_dst, e1_dst, e2_dst]) AS path
    FROM $neighbours
);



$l3 = (
    SELECT
        DISTINCT 
            e0_src AS src,
            e3_dst AS dst,
            ListNotNull([e0_src, e0_dst, e1_dst, e2_dst, e3_dst]) AS path
    FROM $neighbours
);

$passport_inns = (
    SELECT src, dst, path
    FROM (
        SELECT * FROM $l0
        UNION ALL
        SELECT * FROM $l1
        UNION ALL
        SELECT * FROM $l2
        UNION ALL
        SELECT * FROM $l3
    )
    WHERE ($nodeId2Struct(dst).type = 'spark_id') AND (ListLength(path) = ListLength(ToSet(path)))
);


INSERT INTO $result_table WITH TRUNCATE
SELECT * 
FROM $passport_inns

