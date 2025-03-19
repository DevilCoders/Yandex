INSERT INTO $TABLE_NAME$ (key, last_version) VALUES('$KEY$', '$VERSION$') ON CONFLICT(key) DO UPDATE SET last_version=GREATEST(EXCLUDED.last_version, $TABLE_NAME$.last_version);
