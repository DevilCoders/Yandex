DELETE FROM permissions WHERE uid not in (691213473, 609573294, 598686045, 539446614, 686400023, 513858969);

CREATE TABLE public.user_logins (
    uid BIGINT,
    passportlogin character varying(63) NOT NULL,
    internallogin character varying(63) NOT NULL,
    PRIMARY KEY(uid)
);


CREATE INDEX idx_user_logins_uid ON public.user_logins USING btree (uid);

INSERT INTO public.user_logins (uid, passportlogin, internallogin)
VALUES
    (691213473, 'yndx-a-salnikov', 'a-salnikov'),
    (609573294, 'yndx-aab-admin-solovyev', 'solovyev'),
    (598686045, 'yndx-ddemidov-manager',  'ddemidov'),
    (539446614, 'yndx.sotskov', 'sotskov'),
    (686400023, 'ynd-aab-adm-dridgerve', 'dridgerve'),
    (513858969, 'yndx-mavlyutov', 'mavlyutov');


ALTER TABLE ONLY public.permissions
    ADD CONSTRAINT permissions_uid_fkey FOREIGN KEY (uid) REFERENCES public.user_logins(uid);
