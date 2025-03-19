CREATE OR REPLACE FUNCTION code._error_already_registered()
RETURNS text AS $$
BEGIN
    RETURN 'MDD01';
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION code._error_registration_timeout()
RETURNS text AS $$
BEGIN
    RETURN 'MDD02';
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION code._error_not_found()
RETURNS text AS $$
BEGIN
    RETURN 'MDD03';
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION code._error_no_open_master()
RETURNS text AS $$
BEGIN
    RETURN 'MDD04';
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION code._error_invalid_state()
RETURNS text AS $$
BEGIN
    RETURN 'MDD05';
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION code._error_invalid_input()
RETURNS text AS $$
BEGIN
    RETURN 'MDD06';
END;
$$ LANGUAGE plpgsql;
