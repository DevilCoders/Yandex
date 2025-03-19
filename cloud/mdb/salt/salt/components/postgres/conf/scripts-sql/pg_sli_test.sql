BEGIN;

CREATE TABLE IF NOT EXISTS public.pg_sli_test (iid integer);
CREATE UNIQUE INDEX IF NOT EXISTS pg_sli_index ON public.pg_sli_test(iid);
GRANT ALL PRIVILEGES ON TABLE pg_sli_test to monitor ;

COMMIT;
