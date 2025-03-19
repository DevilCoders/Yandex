CREATE OR REPLACE FUNCTION code.commands_def_from_json(json)
RETURNS code.command_def[] AS $$
SELECT ARRAY(
    SELECT (js->>'type',
            ARRAY(SELECT json_array_elements_text(js->'arguments')),
            (js->>'timeout')::interval)::code.command_def
      FROM json_array_elements(json_strip_nulls($1)) js
);
$$ LANGUAGE SQL STABLE;