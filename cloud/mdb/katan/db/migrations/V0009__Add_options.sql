ALTER TABLE katan.schedules
    ADD options jsonb NOT NULL DEFAULT '{}';
ALTER TABLE katan.schedules
    ADD CONSTRAINT check_options_should_be_an_object CHECK (
            jsonb_typeof(options) = 'object'
        );
ALTER TABLE katan.schedules DROP CONSTRAINT check_age_less_then_year;


ALTER TABLE katan.rollouts
    ADD options jsonb NOT NULL DEFAULT '{}';
ALTER TABLE katan.rollouts
    ADD CONSTRAINT check_options_should_be_an_object CHECK (
            jsonb_typeof(options) = 'object'
        );
