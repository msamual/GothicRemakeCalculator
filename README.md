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

Откройте http://localhost:8080/GothicRemakeLockPuzzleCalculator/

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

## Деплой на Ubuntu (GitHub Actions)

Используется **self-hosted runner** на сервере (как в проекте iq): при push в `main` GitHub Actions запускает `deploy.sh` прямо на машине.

### 1. Подготовка сервера

```bash
# Docker + Compose (plugin или standalone)
sudo apt update
sudo apt install -y docker.io curl git
sudo apt install -y docker-compose-plugin || sudo apt install -y docker-compose
sudo usermod -aG docker $USER
# перелогиниться, чтобы группа docker применилась
```

**Важно для self-hosted runner:** пользователь, под которым работает runner (тот, кто запускал `./config.sh`), тоже должен быть в группе `docker`:

```bash
sudo usermod -aG docker <runner-user>
# перезапустить runner (из каталога actions-runner):
sudo ./svc.sh stop && sudo ./svc.sh start
```

Либо настрой passwordless sudo для docker (тогда `deploy.sh` использует `sudo docker` автоматически):

```bash
# /etc/sudoers.d/github-runner-docker
<runner-user> ALL=(ALL) NOPASSWD: /usr/bin/docker, /usr/bin/docker-compose
```

`deploy.sh` автоматически использует `docker compose` или `docker-compose` — что установлено на сервере.

### 2. Self-hosted runner

В GitHub: **Settings → Actions → Runners → New self-hosted runner → Linux**.

На сервере (отдельный пользователь, например `deploy`):

```bash
mkdir -p ~/actions-runner && cd ~/actions-runner
# скачать архив и checksum с страницы GitHub (версия может отличаться)
curl -o actions-runner-linux-x64-2.321.0.tar.gz -L \
  https://github.com/actions/runner/releases/download/v2.321.0/actions-runner-linux-x64-2.321.0.tar.gz
tar xzf ./actions-runner-linux-x64-*.tar.gz
./config.sh --url https://github.com/<owner>/GothicCalculator --token <TOKEN>
sudo ./svc.sh install
sudo ./svc.sh start
```

Runner должен быть **online** перед первым деплоем.

### 3. Workflows

| Workflow | Триггер | Runner | Действие |
|----------|---------|--------|----------|
| `test.yml` | push / PR в `main` | `ubuntu-latest` | `make test`, `dotnet test`, сборка Docker |
| `deploy.yml` | push в `main`, manual | `self-hosted` | `./deploy.sh` |

### 4. Порт и ручной деплой

По умолчанию приложение слушает порт **8080**. Можно переопределить:

```bash
export APP_PORT=8080
./deploy.sh
```

Или добавить в systemd/svc runner'а переменную окружения `APP_PORT`.

### 5. Nginx на msamual.online

Приложение публикуется по пути:

**https://msamual.online/GothicRemakeLockPuzzleCalculator/**

#### Диагностика (на сервере)

```bash
chmod +x deploy/diagnose.sh
./deploy/diagnose.sh
```

#### Установка nginx (обязательно)

Код runner'а лежит **не** в `actions-runner/`, а здесь:

```bash
find /home/github-runner -maxdepth 5 -type d -name GothicRemakeCalculator 2>/dev/null
# обычно: .../_work/GothicRemakeCalculator/GothicRemakeCalculator
```

Установка nginx из checkout:

```bash
cd /home/github-runner/actions-runner/_work/GothicRemakeCalculator/GothicRemakeCalculator
chmod +x deploy/install-nginx.sh
sudo ./deploy/install-nginx.sh
```

Или вручную (если `location` вне `server {}` — nginx не запустится):

```bash
sudo nano /etc/nginx/sites-available/msamual.online
```

Содержимое — только блок `server { ... }` из [`deploy/msamual.online.conf`](deploy/msamual.online.conf) (строки 12–32), **без** `location` отдельно от `server`.

```bash
sudo ln -sf /etc/nginx/sites-available/msamual.online /etc/nginx/sites-enabled/
sudo rm -f /etc/nginx/sites-enabled/default
sudo nginx -t && sudo systemctl reload nginx
```

Проверка:

```bash
curl http://127.0.0.1:8080/GothicRemakeLockPuzzleCalculator/api/lock/health
curl http://msamual.online/GothicRemakeLockPuzzleCalculator/api/lock/health
```

#### HTTPS

Порт 443 должен быть открыт. После того как HTTP заработает:

```bash
sudo apt install -y certbot python3-certbot-nginx
sudo certbot --nginx -d msamual.online
```

Контейнер слушает `:8080` и сам обслуживает подпуть `/GothicRemakeLockPuzzleCalculator/`.

### 6. Проверка после деплоя

```bash
curl http://127.0.0.1:8080/GothicRemakeLockPuzzleCalculator/api/lock/health
curl -k https://msamual.online/GothicRemakeLockPuzzleCalculator/api/lock/health
docker compose ps
docker compose logs -f api
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
