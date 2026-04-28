# Fitness Tracker API on userver

REST API для фитнес-трекера, переписанный на `userver`. Сервис хранит данные в памяти процесса и сохраняет тот же набор прикладных сущностей: пользователи, упражнения, тренировки и bearer-аутентификация.

## Стек

- C++20
- userver, вендорится в репозитории как git submodule
- In-memory storage

## Структура

```text
homework_2/
├── CMakeLists.txt
├── configs/
│   ├── config_vars.yaml
│   └── static_config.yaml
├── third_party/
│   └── userver/
├── src/
│   ├── api_utils.cpp
│   ├── fitness_storage.cpp
│   ├── handlers.cpp
│   └── main.cpp
├── Dockerfile
├── docker-compose.yaml
├── openapi.yaml
└── README.md
```

## Сборка

`userver` не нужно устанавливать в систему отдельно: он подключается из `third_party/userver` как vendored dependency.

После клонирования репозитория обязательно инициализируйте submodule:

```bash
git submodule update --init --recursive
```

Полная локальная сборка:

```bash
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Запуск

```bash
./build/fitness-tracker-userver \
  --config configs/static_config.yaml \
  --config_vars configs/config_vars.yaml
```

Сервис стартует на `http://localhost:8080`.

## Docker

Сборка контейнера использует тот же vendored `userver`, что и локальная сборка, поэтому submodule тоже должен быть инициализирован до запуска Docker build.

```bash
git submodule update --init --recursive
docker-compose up --build
```

## API

| Метод | Endpoint | Описание |
|-------|----------|----------|
| POST | `/register` | Регистрация пользователя |
| POST | `/token` | Получение bearer-токена |
| POST | `/users` | Создание пользователя |
| GET | `/users/search` | Поиск по логину или маске имени/фамилии |
| POST | `/exercises` | Создание упражнения, нужен `Authorization: Bearer <token>` |
| GET | `/exercises` | Список упражнений |
| POST | `/users/{user_id}/workouts` | Создание тренировки |
| POST | `/users/{user_id}/workouts/{workout_id}/exercises` | Добавление упражнения в тренировку |
| GET | `/users/{user_id}/workouts` | История тренировок |
| GET | `/users/{user_id}/workouts/stats` | Статистика за период |

## Примеры

Регистрация:

```bash
curl -X POST http://localhost:8080/register \
  -H "Content-Type: application/json" \
  -d '{
    "username": "john",
    "first_name": "John",
    "last_name": "Doe",
    "email": "john@example.com",
    "password": "secret123"
  }'
```

Логин:

```bash
curl -X POST http://localhost:8080/token \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "username=john&password=secret123"
```

Создание упражнения:

```bash
curl -X POST http://localhost:8080/exercises \
  -H "Authorization: Bearer <TOKEN>" \
  -H "Content-Type: application/json" \
  -d '{
    "name": "Жим лёжа",
    "description": "Базовое упражнение для грудных мышц",
    "muscle_group": "Грудь"
  }'
```

## Примечания

- `openapi.yaml` оставлен как контракт API.
- Данные и токены не переживают перезапуск процесса.
- Если `third_party/userver` отсутствует, `cmake` завершится с подсказкой запустить `git submodule update --init --recursive`.
- Старые Python-тесты удалены, потому что приложение больше не работает как встроенный FastAPI app внутри `pytest`.
