/* allow requested action */
BEGIN TRANSACTION;

UPDATE cms.decisions
SET status = 'ok'
WHERE request_id IN (SELECT id FROM cms.requests WHERE request_ext_id = :'request_id');

UPDATE cms.requests
SET analysed_by = :'your_login'
WHERE request_ext_id = :'request_id';

COMMIT;
