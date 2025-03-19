/* forbid requested action */
BEGIN TRANSACTION;

UPDATE cms.requests
SET resolved_by         = :'your_login',
    analysed_by         = :'your_login',
    status              = 'rejected',
    resolve_explanation = :'explanation' || '. Rejected by staff:' || :'your_login' || '.',
    resolved_at         = now()
WHERE request_ext_id = :'request_id';

UPDATE cms.decisions
SET status = 'at-wall-e'
WHERE request_id IN (SELECT id FROM cms.requests WHERE request_ext_id = :'request_id');
COMMIT;
