CREATE TYPE public.service_support_priority AS ENUM (
    'critical',
    'major',
    'minor',
    'other'
);

ALTER TABLE ONLY public.services ADD COLUMN support_priority public.service_support_priority NOT NULL DEFAULT 'other';

UPDATE public.services SET support_priority='critical'
    WHERE id in ('yandex_morda', 'yandex_zen', 'yandex_news', 'yandex_pogoda', 'yandex_mail');
UPDATE public.services SET support_priority='major'
    WHERE id in ('yandex_tv', 'autoru', 'yandex_images', 'yandex_video', 'yandex_sport', 'music.yandex.ru', 'kinopoisk.ru', 'yandex_realty', 'docviewer.yandex.ru', 'yandex.question');
UPDATE public.services SET support_priority='minor'
    WHERE id in ('yandex_afisha', 'drive2', 'nova.rambler.ru', 'livejournal', 'razlozhi');
