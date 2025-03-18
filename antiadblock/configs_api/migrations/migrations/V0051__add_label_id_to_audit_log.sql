ALTER TABLE public.audit_log ADD COLUMN label_id CHARACTER VARYING(127) DEFAULT NULL;

DROP INDEX idx_audit_entry_service_id_date;

CREATE INDEX idx_audit_entry_label_id_date on public.audit_log USING btree (label_id, date DESC);

