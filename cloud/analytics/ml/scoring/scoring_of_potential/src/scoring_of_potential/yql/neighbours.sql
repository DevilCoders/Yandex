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
           edges1.src as e1_src,
           edges1.dst as e1_dst,
           edges2.src as e2_src,
           edges2.dst as e2_dst,
           edges3.src as e3_src,
           edges3.dst as e3_dst
    FROM $edges AS edges
    INNER JOIN $edges AS edges1
        ON edges.dst = edges1.src
    INNER JOIN $edges AS edges2
        ON edges1.dst = edges2.src
    INNER JOIN $edges AS edges3
        ON edges2.dst = edges3.src
);


INSERT INTO $result_table WITH TRUNCATE 
SELECT *
FROM $neighbours


