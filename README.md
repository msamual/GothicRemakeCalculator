# Gothic Lock Calculator

Калькулятор взлома замков для **Gothic 1 Remake**. Программа принимает описание замка (связи между пластинами и стартовые позиции) и находит **безопасную** последовательность ходов с **минимальным числом строк** в инструкции. При равном числе строк выбирается путь с меньшим общим числом нажатий.

Доступны **консольный солвер** и **веб-интерфейс** (Angular + .NET API).

## Сборка CLI

```bash
make
make test
```

Бинарник: `gothic-lock`

## Использование CLI

```bash
gothic-lock tests/fixtures/tower_chest.txt
gothic-lock --json tests/fixtures/tower_chest.txt
gothic-lock - < my_lock.txt
gothic-lock --template
gothic-lock --help
```

## Веб-интерфейс

### Docker (рекомендуется)

```bash
make docker-up
# или
docker compose up -d --build
```

Откройте http://localhost:8080

### Локальная разработка

Терминал 1 — API:

```bash
make
cd GothicCalculatorApi
dotnet run
```

Терминал 2 — frontend:

```bash
cd gothic-calculator-frontend
npm install
npm start
```

Откройте http://localhost:4200 (запросы `/api` проксируются на http://localhost:5000).

## API

### `GET /api/lock/health`

Проверка состояния API и наличия бинарника солвера.

### `POST /api/lock/solve`

**Request:**

```json
{
  "name": "Second chest in the tower",
  "rules": ["3r, 6l", "-", "1r, 4l, 6r", "2r, 5r, 6l", "-", "3l"],
  "start": [5, 3, 6, 7, 2, 7]
}
```

**Response (успех):**

```json
{
  "ok": true,
  "name": "Second chest in the tower",
  "status": "solved",
  "lines": 18,
  "steps": 52,
  "instructions": [
    { "plate": 1, "count": 1, "direction": "left" }
  ]
}
```

**Response (ошибка):** HTTP 400, `"ok": false`, поле `"error"`.

Переменная окружения `GOTHIC_LOCK_BIN` — путь к бинарнику (по умолчанию `../gothic-lock` относительно API).

## Формат ввода

```
Name: Second chest in the tower
Rules:
1: 3r, 6l
2: -
3: 1r, 4l, 6r
4: 2r, 5r, 6l
5: -
6: 3l
Start:
[5, 3, 6, 7, 2, 7]
```

- **Rules** — по одной строке на пластину. Нажимаете **[D]** (пластина вправо) и записываете, куда сдвинулись *другие* пластины: `Nr` / `Nl`. `-` = никто не двигается. **[A]** даёт противоположный эффект (вычитает ту же строку матрицы).
- **Start** — позиции **пластин** по шкале 1–7: **1 = правый край**, **7 = левый**, цель — **4**. Это формат [nameless-locksmith](https://github.com/Arminek/nameless-locksmith) (удобнее смотреть на пластины, а не на штифты).
- Поддерживается **2–8 пластин**.

## Как снять правила в игре

1. Выберите пластину и нажмите **D** (сдвиг вправо).
2. Посмотрите, какие *другие* пластины сдвинулись и куда.
3. Запишите связи в формате `Nr` / `Nl`.
4. Если пластина упёрлась в стену, сначала сдвиньте её к центру — иначе часть связей не видна.

## Управление в игре

| Клавиша | Действие |
|---------|----------|
| **D** | Пластина вправо |
| **A** | Пластина влево |
| **W** / **S** | Переключение между пластинами |

## Пример вывода CLI

```
Second chest in the tower
Решение (18 строк, 52 шагов):
  1 пластина 1 раз вправо
  ...
```

## Структура проекта

```
GothicCalculator/
├── src/                          # C-солвер
├── GothicCalculatorApi/          # .NET 9 Web API
├── gothic-calculator-frontend/   # Angular SPA
├── docker-compose.yml
└── Dockerfile
```

## Лицензия

MIT
