# Дипломный проект «Поисковая система»

* [Описание](#описание).
* [Программа «Паук»](#программа-паук):
    * [общее описание](#общее-описание),
    * [индексация](#индексация),
    * [переход между страницами](#переход-между-страницами),
    * [программа-поисковик](#программа-поисковик),
    * [база данных](#база-данных),
    * [файл конфигурации ini](#файл-конфигурации-ini).

---
## Рекомендации по работе над дипломом

1. Не откладывайте надолго начало работы над дипломом. В таком случае у вас останется больше времени на получение рекомендаций от руководителя и доработку.
2. Разбейте работу над дипломом на части. Выполняйте их поочерёдно. Вы будете успевать учитывать комментарии от руководителя и не терять мотивацию на полпути.
3. Сдавайте работу на проверку частями. Не ждите, когда выполните всё задание полностью.
---

## Описание
Необходимо написать поисковую систему — аналог Google, Яндекс или Yahoo.

Поисковая система будет получать данные с сайтов, строить поисковые индексы и по запросу выдавать релевантные результаты поисковой выдачи.

Поисковая система будет состоять из следующих частей:
* программа «Паук». Это HTTP-клиент, задача которого — парсить сайты и строить индексы исходя из частоты слов в документах;
* программа-поисковик. Это HTTP-сервер, задача которого — принимать запросы и возвращать результаты поиска.

Для хранения информации следует использовать базу данных PostgreSQL.

Настройки программ следует хранить в ini-файле конфигурации.

# Программа «Паук»
## Общее описание
Программа «Паук» (spider), или «Краулер» (crowler), — это неотъемлемая часть поисковой системы. Эта программа, которая обходит интернет, следуя по ссылкам на веб-страницах. Начиная с одной страницы, она переходит на другие по ссылкам, найденным на этих страницах. «Паук» загружает содержимое каждой страницы, а затем индексирует её.

Изнутри программа «Паук» представляет из себя HTTP-клиент, который переходит по ссылкам, скачивает HTML-страницы и анализирует их содержимое, добавляя записи в базу данных.

## Индексация
После загрузки веб-страницы производится её индексация. Для этого в составе программы «Паук» существует отдельный модуль — индексатор. Индексатор удаляет HTML-теги из страницы, а затем удаляет все знаки препинания, переносы строк, табуляцию, оставляя только чистый текст. Затем индексатор анализирует текст на странице и сохраняет информацию в базе данных для того, чтобы вести поиск по этим данным.

Индексатор выполняет следующие действия для страницы:
* страница очищается от HTML-тегов, от знаков препинания, табуляции, переносов строк и т. д. Остаются только слова, разделённые пробелами;
* слова переводятся в нижний регистр — для этого вы можете использовать библиотеку [Boost Locale](https://www.boost.org/doc/libs/1_82_0/libs/locale/doc/html/index.html). Для простоты можно также отбрасывать все слова короче трёх или длиннее 32 символов;
* анализируется частотность слов в тексте. Индексатор считает, сколько раз встречается каждое слово в тексте;
* информация о словах и о частотности сохраняется в базу данных.

Для хранения информации о частотности слов рекомендуется использовать две таблицы — «Документы» и «Слова» — и реализовать между ними связь «многие-ко-многим» с помощью промежуточной таблицы. В эту же промежуточную таблицу будет записана частота использования слова в документе. Однако конечное решение о том, как хранить данные в базе и какая будет структура базы данных, должны принять вы.

## Переход между страницами
В ini-файле конфигурации задаётся страница, которую необходимо сохранить, и «Паук» начинает свою работу именно с неё.

«Паук» загружает эту страницу и индексирует её.

В HTML-страницах также существуют ссылки, обозначаемые HTML-тегом `<a href="...">`. «Паук» должен переходить по этим ссылкам и скачивать эти страницы тоже. Для этого «Паук» реализует многопоточную очередь и пул потоков. Очередь состоит из ссылок на скачивание, а пул потоков выполняет задачу по скачиванию веб-страниц с этих ссылок и их индексацию. После скачивания очередной страницы «Паук» собирает все ссылки с неё и добавляет их в очередь на скачивание.

Для сборки ссылок вы можете использовать регулярные выражения, например, `"<a href=\"(.*?)\""`. Или вы можете подключить библиотеку для XML-парсинга с поддержкой XPath-запросов и выполнить поиск по запросу `"//a/@href"`.

«Паук» переходит по страницам не бесконечно. Глубина рекурсии должна быть задана настройкой в ini-файле конфигурации в виде числа. К примеру, если глубина рекурсии задана как 1, то «Паук» анализирует только непосредственно стартовую страницу. Если глубина рекурсии равна 2, то «Паук» анализирует стартовую страницу и страницы, на которые она ссылается. Если глубина рекурсии равна 3, то «Паук», анализирует стартовую страницу, страницы, на которые ссылается стартовая страница, и страницы, которые находятся на страницах, на которые стартовая страница, и так далее.

Для реализации HTTP-клиента рекомендуется подключить и использовать библиотеку [Boost Beast]( https://www.boost.org/doc/libs/1_82_0/libs/beast/doc/html/index.html ). 

## Программа-поисковик
Программа-поисковик выполняет поиск по базе данных, ранжирует результат и возвращает его пользователю. В настоящих поисковых системах ранжирование выполняется по множеству критериев, однако для простоты будем ранжировать страницу по частоте упоминания искомых слов в документе.

Изнутри поисковик представляет из себя HTTP-сервер, который принимает два вида запросов: POST и GET. Порт, на котором будет запущен HTTP-сервер, должен взять из ini-файла конфигурации.

По запросу GET открывается простая статическая HTML-страница с формой поиска. На форме должно быть поле ввода, а также кнопка поиска.

По запросу POST происходит извлечение из базы данных результата запроса и его ранжирование, а затем перед пользователем открывается простая статическая веб-страница с результатами поиска.

Для простоты запрос можно анализировать только в виде слов, разделённых пробелами. Все прочие знаки препинания игнорируются. Можно также ограничить длину запроса — не более четырёх слов. 

Для извлечения данных напишите SQL-запрос. Запрос должен быть следующим: взять документы, в которых встречаются все слова из запроса, и отсортировать их по суммарному количеству упоминаний слов в документе.

Приведём пример для запроса «‎привет мир». Предположим, если в документе A слово «‎привет» встречается 10 раз, а слово «‎мир» четыре раза, то этот документ имеет релевантность 10 + 4 = 14. Если в документе B слово «‎привет» встречается три раза, а слово «‎мир» восемь раз, то релевантность его равна 3 + 8 = 11, и он будет находиться ниже в выдаче.

Для простоты можете ограничить выдачу, выдавая не более чем 10 результатов поиска.

После выполнения запроса программа должна вернуть статическую HTML-страницу с результатами поиска — ссылками на веб-страницы. Результаты должны быть отсортированы по убыванию.

В случае если результатов не найдено, на HTML-странице следует отобразить соответствующее сообщение.

В случае если при работе программы возникла ошибка, на HTML-странице также следует отобразить сообщение о внутренней ошибке.

Для реализации HTTP-сервера рекомендуется подключить и использовать библиотеку [Boost Beast]( https://www.boost.org/doc/libs/1_82_0/libs/beast/doc/html/index.html ). 

## База данных
Для выполнения задания рекомендуется использовать базу данных PostgreSQL.

Для подключения к базе данных рекомендуется использовать библиотеку `libpqxx`.

Настройки базы данных (хост, порт, название БД, имя пользователя, пароль) следует хранить в ini-файле конфигурации.

При первом запуске программы «‎Паук» следует выполнить создание таблиц базы данных, используя запрос `CREATE TABLE IF NOT EXISTS`.

## Файл конфигурации ini
Все настройки программ следует хранить в ini-файле. Этот файл должен парситься при запуске вашей программы.

В этом файле следует хранить следующие параметры:
* хост (адрес) базы данных;
* порт, на котором запущена база данных;
* название базы данных;
* имя пользователя для подключения к базе данных;
* пароль пользователя для подключения к базе данных;
* стартовая страница для программы «‎Паук»;
* глубина рекурсии для программы «‎Паук»;
* порт для запуска программы-поисковика.
