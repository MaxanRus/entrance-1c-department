## Для компиляции проекта необходим буст.

Так же использовалась библиотека easylogginpp, но она уже лежит в папочке easylogging

```
mkdir build
cd build
cmake ..
cmake --build
```

Запуск

```
./Server port(default 6666)
./Client filename
./Client ip filename
./Client ip port filename
```

filename - это название файла в папке с сервером

Файл скачается с названием download_filename.

В оба для для написания вступительного были контрольные, поэтому я не успел реализовать передачу данных через несколько сокетов, проверка корректности файла мне не нужна так как я передаю все через TCP.
