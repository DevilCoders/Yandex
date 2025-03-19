ALTER TABLE cms.requests
    ADD resolve_explanation text DEFAULT '' NOT NULL;
ALTER TABLE cms.requests
    ADD CONSTRAINT rejected_with_explanation
        CHECK ((status = 'rejected' AND resolve_explanation != '') OR (status != 'rejected'))
