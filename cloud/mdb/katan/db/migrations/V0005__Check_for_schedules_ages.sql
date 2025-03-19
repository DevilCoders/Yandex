ALTER TABLE katan.schedules ADD
    CONSTRAINT check_age_less_then_year CHECK (
        age < '1 year'
    );
