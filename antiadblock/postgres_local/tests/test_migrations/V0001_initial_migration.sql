CREATE TABLE users(
  id BIGSERIAL PRIMARY KEY,
  name VARCHAR(64) NOT NULL
);

INSERT INTO users(name) VALUES ('test_user');