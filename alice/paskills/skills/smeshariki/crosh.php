<?php

    $quest[0] = [
        "q" => 'Привет, ребята! Представляете, Крош нашёл карту сокровищ! И прямо сейчас мы отправляемся их искать! Хотите с нами? Тогда давайте сначала потренируемся. Я буду задавать вопросы, а вы — громко отвечайте. Первый вопрос: сколько ушей у Кроша?',
        "a" => ['два', '2', 'двое'],
        "b" => ['Подскажи'],
        "img" => '1521359/d438b0ccd2655ea595ed',
        "zvooq" => 'Крош, поздоровайся с ребятами.
        
        <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/1389304e-6807-41b6-aae8-2d4ed2ca14e1.opus">
        
        Чтобы искать сокровища вместе с Кр+ошем, громко отвечайте на вопросы. Будьте внимательны! Если не расслышали или не поняли вопрос, скажите: «Ал+иса, повтори». А если нужна подсказка, кричите: «Ал+иса, подскажи». 
Давайте потренируемся. Первый вопрос: Сколько ушей у Кр+оша?',
        "true" => [
            "desc" => 'Точно – два уха!',
            "zvooq" => '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/717a05ec-e2ce-4cc3-ab99-0578db505173.opus">',
        ],
        "1false" => [
			[
				"a" => [''], 
				"desc" => 'Давайте попробуем ещё раз. У Кроша столько же ушей, сколько у вас. Ну-ка, посчитайте! Сколько получилось?',
				"zvooq" => 'Давайте попробуем ещё раз. У Кроша столько же ушей, сколько у вас. Ну-ка, посчитайте! Сколько получилось?', 
			]
		],
        "3false" => [
            [
                "a" => [''],
                "desc" => 'У Кроша два уха! А вы держите ушки на макушке.',
                "zvooq" => 'У Кроша два +уха! А вы держите ушки на макушке, внимательно слушайте вопросы и отвечайте.',
            ]
        ],
    ];


    $quest[1] = [
        "q" => 'Мы отправляемся за сокровищами! Только куда же идти? На карте нарисованы пальмы, реки, джунгли. Ещё пустыня какая-то… Ребята! Что это за место, где есть и пустыня, и джунгли?', 
        "a" => ['африка', 'танзания'],
        "b" => ['Подскажи'], 
        "img" => '1030494/9dfcbe408de4b2d4b265',
        "zvooq" => 'Мы отправляемся за сокровищами! Только куда же идти? Крош, что нарисовано на твоей карте?
        
        <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/d09289d3-bcb3-43ec-925d-6ea2bded1fc3.opus">
                
        Ребята! Что это за место, где есть и пустыня, и джунгли?',
        "true" => [
            "desc" => 'Правильно. Вы молодцы!', 
            "zvooq" => 'Правильно. Вы молодцы!',
        ],
        "1false" => [
			[
				"a" => [''], 
				"desc" => 'Давайте подумаем. В этом месте очень жарко! А ещё там живут жирафы, слоны, бегемоты и даже верблюды! Что это за место?', 
				"zvooq" => 'Давайте подумаем. В этом месте очень жарко! А ещё там живут жирафы, слоны, бегемоты и даже верблюды! Что это за место?',
			]
		],
        "2false" => [
            [
                "a" => [''], 
                "desc" => 'Попробуйте ещё раз. Туда спешил доктор Айболит, чтобы вылечить больных обезьян. Это слово начинается на букву А! Назовите, что это за место?',
                "zvooq" => 'Попробуйте ещё раз. Туда спешил доктор Айболит, чтобы вылечить больных обезьян. Это слово начинается на букву +А! Назовите, что это за место?',
            ] 
        ],
        "3false" => [
            [
                "a" => [''], 
                "desc" => 'Это же Африка! Самый жаркий континент на Земле.', 
                "zvooq" => 'Это Африка! Самый жаркий континент на Земле.', 
            ]
        ],
    ];


    $quest[] = [
        "q" => 'В Африке лето круглый год. Чтобы добраться до Африки, Крошу нужен быстрый транспорт. Чтобы вжух! — и он на месте. Ребята! Какой транспорт подойдёт Крошу?', 
        "a" => ['самолет'], 
        "b" => ['Подскажи'],
        "img" => '1030494/b71412d6e1cddbac70a7', 
        "zvooq" => 'В Африке лето круглый год. Там живут удивительные звери и растут сладкие фрукты.
        
        <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/8eee67aa-5bc3-422d-b009-56988acabdd5.opus">
        
        Ребята! Какой транспорт подойдёт Кр+ошу?',
        "true" => [
            "desc" => 'Точно! Вы правы! Это самолет! Воздушный транспорт. На нём мы быстро долетим до Африки!', 
            "zvooq" => 'Точно! Вы правы! Это самолет! Воздушный транспорт. На нём мы быстро долетим до Африки!', 
        ],
        "1false" => [
			[
				"a" => ['не знаю'], 
				"desc" => 'Крошу нужен воздушный транспорт с большими крыльями. Что это?', 
				"zvooq" => 'Кр+ошу нужен воздушный транспорт с большими крыльями. Что это?',
            ],
            [
                "a" => [''], 
                "desc" => 'Интересный вариант! Но сейчас нам нужен транспорт, который можно встретить в аэропорту! Как он называется?',
                "zvooq" => 'Интересный вариант! Но сейчас нам нужен транспорт, который можно встретить в аэропорту! Как он называется?', 
            ]
		],
        "3false" => [
            [
                "a" => [''],
                "desc" => 'Это самолет! Воздушный транспорт. На нём мы быстро долетим до Африки!',
                "zvooq" => 'Это самолет! Воздушный транспорт. На нём мы быстро долетим до Африки!',
            ]
        ],
    ];

    
    $quest[] = [
        "q" => 'Ребята, готовы лететь на поиски сокровищ?', 
        "a" => ['да', 'готов', 'конечно', 'так точно'],
        "b" => ['Да'], 
        "img" => '213044/97df9be9260602e163ab',
        "zvooq" => '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/aeb753ea-5ad7-40d2-8f9d-ad31bb757f89.opus">
        
        Ребята, готовы лететь на поиски сокровищ',
        "true" => [
            "desc" => 'А вот и Африка! Как здесь красиво.', 
            "zvooq" => '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/0c7826d0-7357-4bc8-a634-549f51ca53a9.opus"> А вот и Африка! Как здесь красиво!', 
        ],
        "3false" => [
            [
                "a" => [''], 
                "desc" => 'Друзья, сокровища сами себя не найдут! Летим с нами! А вот и Африка! Как здесь красиво.', 
                "zvooq" => '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/78faeea5-6959-428e-9568-40df73d72afd.opus"> <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/0c7826d0-7357-4bc8-a634-549f51ca53a9.opus"> А вот и Африка! Как здесь красиво! ',
            ]
        ],
    ];

    
    $quest[] = [
        "q" => 'И жарко! Хорошо, что есть речка. Ой! Кажется, в реке кто-то затаился. Кто-то зелёный, огромный и страшный. Назовите, кто это?', 
        "a" => ['крокодил', 'аллигатор'], 
        "b" => ['Подскажи'], 
        "img" => '', 
        "zvooq" => '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/d293bc13-ee81-4ddd-81bf-d15faed2685f.opus">
        
        Кажется, в реке кто-то затаился. Кто-то зелёный, огромный и страшный. Назовите, кто это?', 
        "true" => [
            "desc" => 'Точно, вы молодцы! Это крокодил! Он живёт в реках Африки и очень опасен.', 
            "zvooq" => 'Точно, вы молодцы! Это крокодил! Он живёт в реках Африки и очень опасен. Крокодил нападает быстро и неожиданно – выпрыгивает из реки и съедает свою жертву!', 
        ],
        "1false" => [
			[
				"a" => ['дельфин', 'кит', 'рыб', 'осьминог', 'акул'], 
				"desc" => 'Нет, ребята, это кто-то пострашнее, чем '.$split.'! У него огромная пасть с острыми зубами и длинный хвост. Кто это?', 
				"zvooq" => 'Нет, ребята, это кто-то пострашнее, чем '.$split.'! У него огромная пасть с острыми зубами и длинный хвост. Кто это?', 
			]
		],
        "2false" => [
            [
                "a" => [''], 
                "desc" => 'Подсказка! В одном мультике этот зверь был другом Чебурашки и звали его Гена. Что это за зверь?', 
                "zvooq" => 'Подсказка! В одном мультике этот зверь был другом Чебурашки и звали его Гена. Что это за зверь?', 
            ]
        ],
        "3false" => [
            [
                "a" => [''], 
                "desc" => 'В реке притаился крокодил. Он очень опасен!', 
                "zvooq" => 'В реке притаился крокодил! Он очень опасен – может выпрыгнуть из реки и тут же съесть свою жертву!', 
            ]
        ],
    ];

    
    $quest[] = [
        "q" => 'Лучше здесь не купаться. А вообще, в Африке классно! Лето, солнце… И какое-то жужжащее насекомое. Ребята, помогите вспомнить, как это жужжащее насекомое называется?', 
        "a" => ['муха', 'цеце', 'цокотуха'], 
        "b" => ['Подскажи'], 
        "img" => '1521359/29add42a735afd01ada2',
        "zvooq" => '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/fe7500fb-2669-42d5-bb97-b73d61b95d38.opus">
        
        Крош, замри! Ребята, и вы замрите. Перед нами опасное насекомое!
        
        
        <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/b0df53be-dd3e-40e1-a205-e9f0a6beb774.opus">', 
        "true" => [
            "desc" => 'Молодцы, это муха. Но не простая, а муха цеце! Она живёт в Африке и очень опасная!', 
            "zvooq" => 'Какие вы любознательные! Молодцы, это муха. Но не простая, а муха цеце! Она живёт в Африке и очень опасная: если укусит – тут же заболеешь!', 
        ],
        "1false" => [
			[
				"a" => ['комар', 'жук', 'пчела', 'оса', 'шмель', 'мошка', 'овод', 'слепень'], 
				"desc" => 'Точно, '.$split.' тоже жужжит. Но тут кто-то другой. Про это насекомое стишок есть – оно ходило на базар и купило самовар! Скажите, кто это?', 
				"zvooq" => 'Точно, '.$split.' тоже жужжит. Но тут кто-то другой. Про это насекомое стишок есть – оно ходило на базар и купило самовар! Скажите, кто это?', 
            ]
        ],
        "2false" => [
            [
                "a" => [''], 
                "desc" => 'Вы точно знаете это насекомое! Оно любит сладкое! Стоит пролить варенье, и оно тут как тут. Назовите, кто это?', 
                "zvooq" => 'Вы точно знаете это насекомое! Оно любит сладкое! Стоит пролить варенье, и оно тут как тут. Назовите, кто это?', 
            ]
        ],
        "3false" => [
            [
                "a" => [''], 
                "desc" => 'Это муха! Но не простая, а муха цеце! Она живёт в Африке и очень опасная!', 
                "zvooq" => 'Это муха! Но не простая, а муха цеце! Она живёт в Африке и очень опасная: если укусит – тут же заболеешь!', 
            ]
        ],
    ];

    
    $quest[] = [
        "q" => 'Как теперь от этой мухи отвязаться?! У меня идея! Есть животное, на которое муха цеце не нападает. Крошу надо притвориться этим животным! Оно всё полосатое! Ребята, кто это?', 
        "a" => ['зебра'], 
        "b" => ['Подскажи'], 
        "img" => '1030494/1dcc7edabb2e930c2e74', 
        "zvooq" => '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/0578ca75-0b1a-4f8a-b27d-45057de458e4.opus">
        
        У меня идея! Есть животное, на которое муха цеце не нападает. Кр+ошу надо притвориться этим животным! Оно всё полосатое! Ребята, догадайтесь, кто это?', 
        "true" => [
            "desc" => 'Умницы, это действительно зебра!', 
            "zvooq" => 'Умницы, это действительно зебра! Учёные считают, что от её черно-белых полосок у мухи цеце рябит в глазах. Муха не видит зебру и не кусает.', 
        ],
        "1false" => [
			[
				"a" => ['тигр'], 
				"desc" => 'Да, у тигра тоже есть полоски! Но у нашего животного полоски чёрные и белые. Назовите, кто это?', 
				"zvooq" => 'Да, у тигра тоже есть полоски! Но у нашего животного полоски чёрные и белые. Назовите, кто это?', 
            ],
            [
				"a" => ['кошка'], 
				"desc" => 'Да, у кошки тоже есть полоски! Но у нашего животного полоски чёрные и белые. Назовите, кто это?', 
				"zvooq" => 'Да, у кошки тоже есть полоски! Но у нашего животного полоски чёрные и белые. Назовите, кто это?', 
            ],
            [
				"a" => ['шмель'], 
				"desc" => 'Да, у шмеля тоже есть полоски! Но у нашего животного полоски чёрные и белые. Назовите, кто это?', 
				"zvooq" => 'Да, у шмеля тоже есть полоски! Но у нашего животного полоски чёрные и белые. Назовите, кто это?', 
            ],
            [
				"a" => ['пчела'], 
				"desc" => 'Да, у пчелы тоже есть полоски! Но у нашего животного полоски чёрные и белые. Назовите, кто это?', 
				"zvooq" => 'Да, у пчелы тоже есть полоски! Но у нашего животного полоски чёрные и белые. Назовите, кто это?', 
            ],
            [
				"a" => ['оса'], 
				"desc" => 'Да, у осы тоже есть полоски! Но у нашего животного полоски чёрные и белые. Назовите, кто это?', 
				"zvooq" => 'Да, у осы тоже есть полоски! Но у нашего животного полоски чёрные и белые. Назовите, кто это?', 
            ]
        ],
        "2false" => [
            [
                "a" => [''], 
                "desc" => 'Подсказка! Это животное похоже на лошадь, только с полосками и живёт в Африке. Кто это?', 
                "zvooq" => 'Подсказка! Это животное похоже на лошадь, только с полосками и живёт в Африке. Кто это?', 
            ]
        ],
        "3false" => [
            [
                "a" => [''], 
                "desc" => 'Это зебра – полосатая чёрно-белая лошадь.', 
                "zvooq" => 'Это зебра – полосатая чёрно-белая лошадь. Учёные считают, что от её полосок у мухи цеце рябит в глазах. Муха не видит зебру и не кусает.', 
            ]
        ],
    ];

    
    $quest[] = [
        "q" => 'Крош раскрасил себя как зебру. Теперь мухи цеце ему не страшны. Ой! Крош, что с тобой? Кажется, ему нужно охладиться. Ребята, поможем Крошу! Как называется штука, которой можно обмахиваться в жару?', 
        "a" => ['веер', 'вер'], 
        "b" => ['Подскажи'], 
        "img" => '1521359/3f66cb5d3cf44d0a5348', 
        "zvooq" => 'Крош раскрасил себя как зебру. Теперь мухи цеце ему не страшны. Можно бежать за сокровищами! Ой! Крош, что с тобой?
        
        <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/b770ef10-429b-4d69-880e-2e2401d01168.opus">
                
        Бедняга! Тебе нужно охладиться. Ребята, поможем Кр+ошу! Как называется штука, которой можно обмахиваться в жару?', 
        "true" => [
            "desc" => 'Угадали! Это веер!', 
            "zvooq" => 'Угадали! Это веер! Его делают из бумаги или ткани. Когда машешь веером, появляется ветерок и обдувает тебя. В жару – самое то!
            
            <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/0554678c-2de7-451f-b2d2-2886e1406fef.opus">',
        ],
        "1false" => [
			[
				"a" => [''], 
				"desc" => 'Давайте подумаем. Этим предметом обмахивались дамы на балу в давние времена! Как он называется?', 
				"zvooq" => 'Давайте подумаем. Этим предметом обмахивались дамы на балу в давние времена! Как он называется?', 
			]
		],
        "3false" => [
            [
                "a" => [''], 
                "desc" => 'Это же веер, ребята! Крош сделал веер из тропического листка. И готов искать сокровища.', 
                "zvooq" => 'Это веер, ребята! Когда машешь веером, появляется ветерок, и становится свежо! В жару – самое то!
                
                <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/0554678c-2de7-451f-b2d2-2886e1406fef.opus">', 
            ]
        ],
    ];

    
    $quest[] = [
        "q" => 'Куда же идти дальше? Эх, плохо видно карту — темнеет. Надо сделать остановку на ночь. А для этого нам нужен походный домик. Ребята, как он называется?', 
        "a" => ['палатка'], 
        "b" => ['Подскажи'], 
        "img" => '1030494/23b86fb82544e289ffea', 
        "zvooq" => 'Что у нас там по карте, Крош? Куда идти?
        
        <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/1b65873c-186a-4ad7-99b5-a82ee953bd46.opus"> <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/babd8dc5-5151-4f96-87cb-906eab395268.opus">
        
        Да, в Африке темнеет очень быстро! Скоро ты даже ушей своих не увидишь! Надо сделать остановку н+аночь. А для этого нам нужен походный домик. Ребята, как он называется?', 
        "true" => [
            "desc" => 'Молодчина, это действительно палатка! Мы с Крошем ставим палатку и забираемся туда на ночь. Продолжим приключение утром. Спокойной ночи!', 
            "zvooq" => 'Молодчина, это действительно палатка! Мы с Крошем ставим палатку и забираемся туда н+аночь. Продолжим приключение утром. Спокойной ночи!', 
        ],
        "1false" => [
			[
				"a" => [''], 
				"desc" => 'Давайте подумаем вместе. Это такой домик из ткани. Разложил его на природе - и можно ночевать. Ребята, что это?', 
				"zvooq" => 'Давайте подумаем вместе. Это такой домик из ткани. Разложил его на природе - и можно ночевать. Ребята, что это?', 
			]
		],
        "3false" => [
            [
                "a" => [''], 
                "desc" => 'Это палатка – специальный походный домик. Мы с Крошем ставим палатку и забираемся туда на ночь. Продолжим приключение утром. Спокойной ночи!', 
                "zvooq" => 'Давайте я расскажу. Это палатка – специальный походный домик. Мы с Крошем ставим палатку и забираемся туда н+аночь. Продолжим приключение утром. Спокойной ночи!', 
            ]
        ],
    ];

    
    $quest[] = [
        "q" => 'Ёлки-иголки! Что это за звук? Кажется, это рычит какой-то дикий зверь. Ребята, как думаете, кто это там?', 
        "a" => ['лев', 'львица'], 
        "b" => ['Подскажи'], 
        "img" => '1540737/ac1e1e7390ab679aea3c', 
        "zvooq" => '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/837a5775-62b9-434b-a3f3-2f03f1a9060c.opus">
        
        <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/b8efa44b-0261-4ab8-8c2d-b3b4ca3d1993.opus">
        
        Кажется, это рычит какой-то дикий зверь. Гляди, в темноте светятся его жёлтые глаза. Ребята, как думаете, кто это там?', 
        "true" => [
            "desc" => 'Да, это лев! Крошу лучше затаиться и подождать, пока лев пройдёт мимо.', 
            "zvooq" => 'Отличный ответ! Да, это лев! Кр+ошу лучше затаиться и не выход+ить из палатки. Тогда лев пройдёт мимо.', 
        ],
        "1false" => [
			[
				"a" => [''], 
				"desc" => 'Подсказка! Это большой дикий кот с пушистой гривой! Как его зовут?', 
				"zvooq" => 'Подсказка! Это большой дикий кот с пушистой гривой! Как его зовут?', 
			]
		],
        "2false" => [
            [
                "a" => ['тигр', 'пантера', 'леопард', 'гепард', 'рысь', 'ягуар', 'пума'], 
                "desc" => 'Да-а-а-а, '.$split.' – тоже дикая кошка. Но наш ночной гость – самый главный. Он царь зверей! Кто же это?', 
                "zvooq" => 'Даааа, '.$split.' – тоже дикая кошка. Но наш ночной гость – самый главный. Он царь зверей! Кто же это?', 
            ]
        ],
        "3false" => [
            [
                "a" => [''], 
                "desc" => 'Это лев! Крошу лучше затаиться и подождать, пока лев пройдёт мимо.', 
                "zvooq" => 'Это лев! Кр+ошу лучше затаиться и не выход+ить из палатки. Тогда лев пройдёт мимо.', 
            ]
        ],
    ];

    
    $quest[] = [
        "q" => 'Лев ушёл, отправляемся дальше! А вот и озеро. Тут прямо по воде ходят какие-то важные птицы. Розовые — как наша Нюша! Ребята, вы знаете, что это за розовые птицы? Как они называются?',
        "a" => ['фламинго'], 
        "b" => ['Подскажи'], 
        "img" => '965417/4aae753c3cef55b0d5ee', 
        "zvooq" => '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/ab030cbb-f7ed-45e4-aa6e-aa05b0671480.opus">
        
        Вот и наступило утро! Пора отправляться дальше – сокровища ждут! Крош, ты что-то говорил про озеро…
        
        <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/7ca3d2f1-c9cd-4bc4-8f3b-c5c9af426291.opus">
            
        Ребята, вы знаете что это за розовые птицы? Как они называются?', 
        "true" => [
            "desc" => 'Правильно! Это фламинго! Их перья розового цвета, потому что фламинго едят водоросли, в которых содержится краситель.', 
            "zvooq" => 'Правильно! Вы молодцы! Да, не все знают этих птиц! Это фламинго! Их перья розового цвета, потому что фламинго едят редкие водоросли! В этих водорослях есть краситель, который окрашивает птиц в розовый цвет.', 
        ],
        "3false" => [
            [
                "a" => [''], 
                "desc" => 'Это фламинго! Их перья розового цвета, потому что фламинго едят водоросли, в которых содержится краситель.', 
                "zvooq" => 'Да, не все знают этих птиц! Это фламинго! Их перья розового цвета, потому что фламинго едят редкие водоросли! В этих водорослях есть краситель, который окрашивает птиц в розовый цвет.', 
            ]
        ],
    ];

    
    $quest[] = [
        "q" => 'Кстати, этот же краситель есть в красных и оранжевых овощах и фруктах. Друзья! Какие вы знаете овощи и фрукты красного и оранжевого цвета?', 
        "a" => ['не знаю', 'подска'], 
        "b" => ['Подскажи'], 
        "img" => '1540737/f2b733fce213d9438e6f', 
        "zvooq" => '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/130d73bc-2945-4b52-94f7-0bf7ea3c9721.opus">
        
        Нет, этот краситель только на фламинго так действует. Кстати, он есть не только в водорослях. Этот же краситель есть во всех красных и оранжевых овощах и фруктах. Друзья! Какие есть овощи и фрукты красного и оранжевого цвета?', 
        "true" => [
            "desc" => 'А я знаю: морковка, персик, помидор…', 
            "zvooq" => '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/ee9b49fe-c61d-4819-8c2a-bc6ac77675dc.opus">', 
        ],
        "3false" => [
            [
                "a" => [''], 
                "desc" => 'Да, таких овощей и фруктов много! Морковка, помидор, персик и другие.', 
                "zvooq" => 'Да, таких овощей и фруктов много, ребята! Морковка, помидор, красный перец, персик и другие. Очень полезная еда!', 
            ]
        ],
    ];

    
    $quest[] = [
        "q" => 'Так, сверимся с картой! Перед нами Килиманджаро — самая высокая гора в Африке. И не просто гора! Иногда она просыпается и извергается! Ребята, как называется гора, которая так умеет?', 
        "a" => ['вулкан'], 
        "b" => ['Подскажи'], 
        "img" => '1030494/9616b154b31197ac493d', 
        "zvooq" => '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/7994bfea-537c-4fd9-9437-24778ac30f35.opus">
        
        Перед нами Килиманджаро - самая высокая гора в Африке. И не просто гора! Иногда она просыпается и извергается! Ребята, как называется гора, которая так умеет?', 
        "true" => [
            "desc" => 'Умницы! Это вулкан!', 
            "zvooq" => 'Умницы! Это вулкан! Внутри вулкана есть кратер, а в кратэре – лава. Лава очень-очень горячая. Лучше держаться подальше, чтобы не обжечься.', 
        ],
        "1false" => [
			[
				"a" => [''], 
				"desc" => 'Вы близки к разгадке! Когда такая гора просыпается, она начинает заливать всё вокруг раскалённой лавой! Назовите, что это?', 
				"zvooq" => 'Вы близки к разгадке! Когда такая гора просыпается, она начинает заливать всё вокруг раскалённой лавой! Назовите, что это?', 
			]
		],
        "2false" => [
            [
                "a" => [''], 
                "desc" => 'Это слово начинается на букву "В"! Как называется такая гора?', 
                "zvooq" => '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/7c14982a-cb9e-4c3a-9b45-afa06e2f2357.opus">', 
            ]
        ],
        "3false" => [
            [
                "a" => [''], 
                "desc" => 'Тогда я скажу. Это вулкан!', 
                "zvooq" => 'Ладно, тогда я скажу. Это вулкан! Гора, внутри которой прячется лава. Лава очень-очень горячая. Лучше держаться подальше, чтобы не обжечься!', 
            ]
        ],
    ];

    
    $quest[] = [
        "q" => 'Сегодня Килиманджаро спит — извергаться не будет. Можно забираться наверх! Только как? В рюкзаке Кроша есть длинный предмет. Им можно привязать себя к горе и подниматься, выше и выше! Друзья! Догадались, что за длинный предмет?', 
        "a" => ['веревка', 'канат', 'трос'], 
        "b" => ['Подскажи'], 
        "img" => '937455/e50c6189f1e20f5d10d9', 
        "zvooq" => 'Нам повезло! Сегодня гора Килиманджаро спит – извергаться н+е б+удет. Крош, взбирайся наверх!
        
        <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/7f5b4a4c-67d3-4c75-a1f0-159174280b32.opus">
        
        Я знаю, что поможет Кр+ошу. В рюкзаке есть длинный предмет. Им можно привязать себя к горе и подниматься, выше и выше! Друзья! Догадались, что за длинный предмет?', 
        "true" => [
            "desc" => 'Блестящий ответ!', 
            "zvooq" => 'Ну конечно, это верёвка! Блестящий ответ! Крош обмотается верёвкой, привяжет её к большому камню и заберётся на гору!', 
        ],
        "1false" => [
			[
				"a" => [''], 
				"desc" => 'Даю подсказку! С помощью этой штуки Копатыч опускает ведро в колодец, чтобы набрать воды. Что это?', 
				"zvooq" => 'Даю подсказку! С помощью этой штуки Копатыч опускает ведро в колодец, чтобы набрать воды. Что это?', 
			]
		],
        "2false" => [
            [
                "a" => [''], 
                "desc" => 'Почти угадали! Её можно привязать к машинке и катить её за собой! Что это?', 
                "zvooq" => 'Почти угадали! Её можно привязать к машинке и катить её за собой! Что это?', 
            ]
        ],
        "3false" => [
            [
                "a" => [''], 
                "desc" => 'Это верёвка!', 
                "zvooq" => 'Это верёвка! Полезная вещь для искателей сокровищ! Крош обмотается верёвкой, привяжет её к большому камню и заберётся на гору!', 
            ]
        ],
    ];

    
    $quest[] = [
        "q" => 'Крош забрался на вершину горы Килиманджаро. И нашёл… Сундук с сокровищами! Ура! Но сундук закрыт. Чтобы его открыть, надо разгадать пароль. Пароль — это первое животное, которое Крош встретил в Африке. Ребята, помогайте! Кто это был?', 
        "a" => ['крокодил'], 
        "b" => ['Подскажи'], 
        "img" => '1521359/7484fdcc22ab13f709c0', 
        "zvooq" => '<speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/a562fbae-1bdc-40f7-a2cf-6d510e92e35d.opus">
        
        Ура! Мы нашли сундук с сокровищами! Но сундук закрыт! А на крышке нарисовано животное, которое Крош встретил в Африке. Если назвать его громко, сундук откроется! Ребята, помогайте! Назовите первое животное, которое Крош встретил в Африке! Кто это был?',
        "true" => [
            "desc" => 'Молодцы, вы угадали пароль!', 
            "zvooq" => 'Молодцы, вы угадали пароль!', 
        ],
        "1false" => [
			[
				"a" => [''], 
				"desc" => 'Давайте вспомним! Крош сошел с самолёта и увидел речку. Хотел в ней искупаться, но увидел кого-то страшного. Кто там был?', 
				"zvooq" => 'Давайте вспомним! Крош сошел с самолёта и увидел речку. Хотел в ней искупаться, но увидел кого-то страшного. Кто там был?', 
			]
		],
        "3false" => [
            [
                "a" => [''], 
                "desc" => 'Это был крокодил! Значит, пароль на сундуке – крокодил!', 
                "zvooq" => 'Это был крокодил! Значит, пароль на сундуке – крокодил!', 
            ]
        ],
    ];

    
    $quest[] = [
        "q" => 'А теперь — самое интересное! Сейчас мы узнаем, что за сокровище спрятано в сундуке! Крышка сундука открывается… Ребята, как думаете, какое сокровище нашёл Крош? Что там?', 
        "a" => ['аааааааааааа'], 
        "b" => ['Подскажи'], 
        "img" => '1521359/29add42a735afd01ada2', 
        "zvooq" => 'А теперь – самое интересное! Сейчас мы узнаем, что за сокровище спрятано в сундуке! Крышка сундука открывается… Крош, смотри скорее, что же там внутри?
        
        <speaker audio="dialogs-upload/34bdd668-1628-41a3-997e-798f9f0392e9/6bac9091-9706-485f-98c9-f8481d84825d.opus">
                
        Ребята, как думаете, какое сокровище нашёл Крош? Что там?', 
        "true" => [
            "desc" => '', 
            "zvooq" => '', 
        ],
        "1false" => [
			[
				"a" => ['не знаю', 'подсказка', 'подскажи', 'помоги', 'помоги', 'что', 'какая', 'какой'], 
				"desc" => 'Попробуй представить, что тебе бы хотелось найти больше всего на свете! Назови самое классное сокровище!', 
				"zvooq" => 'Попробуй представить, какое сокровище тебе бы хотелось найти больше всего на свете! Гора мороженного, новая игрушка, старинные монеты или что-то ещё? Назови самое классное сокровище!', 
			]
		],
        "3false" => [
            [
                "a" => [''], 
                "desc" => 'Точно! В сундуке '.$split.' и медаль. На ней написано «Самым отважным путешественникам!» Ребята, какие вы молодцы! Помогли Крошу отыскать сокровище, а заодно познакомились с Африкой. Ну а наше приключение подошло к концу. Пока-пока!', 
                "zvooq" => 'Точно! В сундуке '.$split.' и медаль. На ней написано «Самым отважным путешественникам!» <speaker audio="dialogs-upload/ee8d181f-217c-49b2-9933-4152ddbb1417/5e1e1ce4-c206-4abb-8d42-cd13e7261d18.opus">
                
                Ребята, какие вы молодцы! Помогли Кр+ошу отыскать сокровище, а заодно познакомились с Африкой. Ну а сейчас наше приключение подошло к концу. Пока-пока!', 
            ]
        ],
    ];


    $quest[16] = [
        "q" => '', 
        "a" => [], 
        "img" => '213044/a3b5c713a14e24390129', 
        "zvooq" => '', 
        "true" => [
            "desc" => '', 
            "zvooq" => '',
        ],
        "1false" => [
			[
				"a" => [''], 
				"desc" => '', 
				"zvooq" => '', 
			]
		],
        "3false" => [
            [
                "a" => [''], 
                "desc" => '', 
                "zvooq" => '', 
            ]
        ],
    ];    

?>
