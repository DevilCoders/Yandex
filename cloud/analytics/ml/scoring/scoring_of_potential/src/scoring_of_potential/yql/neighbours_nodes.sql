USE hahn;

$edges_path = '{edges_path}';
$result_table = '{result_table}';

$edges = (
        SELECT
            `src`,
            `dst`
        FROM $edges_path
        
        UNION ALL
        
        SELECT
            `dst` AS src,
            `src` AS dst
        FROM $edges_path
        
);


$neighbours = (
    SELECT edges.src  as e0_src,
           edges.dst  as e0_dst,
           edges1.dst as e1_dst,
           edges2.dst as e2_dst,
           edges3.dst as e3_dst,
           edges4.dst as e4_dst,
           edges5.dst as e5_dst
    FROM $edges AS edges
    INNER JOIN $edges AS edges1
        ON edges.dst = edges1.src
    INNER JOIN $edges AS edges2
        ON edges1.dst = edges2.src
    INNER JOIN $edges AS edges3
        ON edges2.dst = edges3.src
    INNER JOIN $edges AS edges4
        ON edges3.dst = edges4.src
    INNER JOIN $edges AS edges5
        ON edges5.dst = edges5.src
);


INSERT INTO $result_table WITH TRUNCATE 
SELECT *
FROM $neighbours


