UPDATE public.configs set created = created - make_interval(hours => 3) WHERE created > make_timestamp(2018, 3, 30,22,0,0); --after releasing MVP-1
UPDATE public.audit_log set date = date - make_interval(hours => 3) WHERE date > make_timestamp(2018, 3, 30,22,0,0); --after releasing MVP-1
