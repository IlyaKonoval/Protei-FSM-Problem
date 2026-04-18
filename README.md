# Protei-FSM-Problem

Анализатор логов FSM: находит «зависшие» машины — те, что на конец лога не
достигли терминального состояния из `end_states.txt`, и выдаёт по ним
CSV-отчёт.

## Сборка (нативно под Linux)

```bash
# Release (-O3 -static)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 

# Debug (ASan + UBSan + LSan, -Wall -Wextra)
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j
```

## Сборка через Docker (если хост — Windows/macOS)

```bash
docker build -t protei-fsm .
# Бинарники будут внутри образа:
#   /src/build-release/app
#   /src/build-debug/app
```

Пример запуска с подмонтированным репозиторием:

```bash
docker run --rm -v "$PWD:/data" protei-fsm \
  /src/build-release/app /data/test_ds1/end_states.txt /tmp/out.csv /data/test_ds1/in.txt
```

## Использование

```
app <end_states.txt> <out.csv> <log1> [<log2> ...]
```

- `end_states.txt` — список терминальных состояний в формате `Имя : State`
  или `Имя:State` (по одной паре на строку). Имя может быть как полным
  (`Sg.SIP.UA`), так и классом (`RegisterLogic`) — в последнем случае
  сопоставляются все инстансы вида `RegisterLogic.<suffix>`.
- `out.csv` — путь к выходному файлу.
- `log1 log2 …` — один или несколько входных логов; обрабатываются по
  порядку, состояния накапливаются.
- Прогресс печатается в `stderr`.

### Формат выходного CSV

```
<last_ts>,<fsm_name>,<id>,<state>,<last_event>,<HH:MM:SS.mmm>
```

- `last_ts` — таймстемп последнего изменения состояния
- `last_event` — последнее входящее сообщение (из `>`-строки)
- Длительность — от `last_ts` до последнего таймстемпа в обработанных
  файлах

## Проверка на фикстурах

```bash
for d in test_ds1 test_ds2 datasets/3 datasets/4; do
  ./build/app "$d/end_states.txt" "/tmp/$(basename $d).csv" "$d/in.txt"
done
./build/app datasets/5/end_states.txt /tmp/ds5.csv \
  datasets/5/in1.txt datasets/5/in2.txt datasets/5/in3.txt
```

## Замечания по реализации

- Зависимости — только STL и POSIX (`stat`, `timegm`). `std::regex` не
  используется: строки разбираются вручную, что заметно быстрее на больших
  логах.
- FSM, чьё имя не подпадает ни под один класс из `end_states.txt`,
  пропускаются сразу же — map активных FSM не растёт на «посторонних»
  логиках.
- Debug-сборка включает `-fsanitize=address,undefined,leak`; на всех
  входящих в репозиторий датасетах ASan/UBSan/LSan не дают диагностик.
