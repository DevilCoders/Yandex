UPDATE public.services SET domain = 'pogoda.l7test.yandex.ru' WHERE domain = 'l7test.yandex.ru/pogoda';
UPDATE public.services SET domain = 'search.yandex.ru' WHERE domain = 'yandex.ru/search'; -- no such domain search.yandex.ru =(
UPDATE public.services SET domain = 'pogoda.yandex.ru' WHERE domain = 'yandex.ru/pogoda';
UPDATE public.services SET domain = 'test.local' WHERE domain = 'yandex.ru/test';
UPDATE public.services SET domain = 'video.yandex.ru' WHERE domain = 'yandex.ru/video';
