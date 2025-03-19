DO $$
DECLARE
    l_part_count integer;
BEGIN
    SELECT count(*) INTO l_part_count FROM plproxy.parts;
     IF l_part_count = 0 THEN
       PERFORM plproxy.update_remote_tables();
     END IF;
END$$;
