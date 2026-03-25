# Fitness Tracker API

REST API для фитнес-трекера (Вариант 14). Управление пользователями, упражнениями и тренировками.

## Стек

- Python 3.11
- FastAPI
- JWT аутентификация (python-jose, passlib)
- In-memory хранилище

## Запуск

### Локально

```bash
pip install -r requirements.txt
python main.py
```

### Docker

```bash
docker-compose up --build
```

Сервер запускается на `http://localhost:8000`.

## Документация

- Swagger UI: http://localhost:8000/docs
- OpenAPI JSON: http://localhost:8000/openapi.json
- OpenAPI YAML: `openapi.yaml`

## API Endpoints

### Аутентификация

| Метод | Endpoint | Описание |
|-------|----------|----------|
| POST | `/register` | Регистрация пользователя |
| POST | `/token` | Получение JWT токена |

### Пользователи

| Метод | Endpoint | Описание |
|-------|----------|----------|
| POST | `/users` | Создание пользователя |
| GET | `/users/search` | Поиск по логину или маске имени/фамилии |

### Упражнения (требуется аутентификация)

| Метод | Endpoint | Описание |
|-------|----------|----------|
| POST | `/exercises` | Создание упражнения |
| GET | `/exercises` | Список упражнений |

### Тренировки (требуется аутентификация)

| Метод | Endpoint | Описание |
|-------|----------|----------|
| POST | `/users/{user_id}/workouts` | Создание тренировки |
| POST | `/users/{user_id}/workouts/{workout_id}/exercises` | Добавление упражнения в тренировку |
| GET | `/users/{user_id}/workouts` | История тренировок |
| GET | `/users/{user_id}/workouts/stats` | Статистика за период |

## Примеры использования

### 1. Регистрация

```bash
curl -X POST http://localhost:8000/register \
  -H "Content-Type: application/json" \
  -d '{
    "username": "john",
    "first_name": "John",
    "last_name": "Doe",
    "email": "john@example.com",
    "password": "secret123"
  }'
```

### 2. Получение токена

```bash
curl -X POST http://localhost:8000/token \
  -d "username=john&password=secret123"
```

Ответ:
```json
{"access_token": "eyJhbGciOiJIUzI1NiIs...", "token_type": "bearer"}
```

### 3. Создание упражнения

```bash
curl -X POST http://localhost:8000/exercises \
  -H "Authorization: Bearer <TOKEN>" \
  -H "Content-Type: application/json" \
  -d '{
    "name": "Жим лёжа",
    "description": "Базовое упражнение для грудных мышц",
    "muscle_group": "Грудь"
  }'
```

### 4. Создание тренировки

```bash
curl -X POST http://localhost:8000/users/<USER_ID>/workouts \
  -H "Authorization: Bearer <TOKEN>" \
  -H "Content-Type: application/json" \
  -d '{"name": "Тренировка груди", "date": "2024-03-20"}'
```

### 5. Добавление упражнения в тренировку

```bash
curl -X POST http://localhost:8000/users/<USER_ID>/workouts/<WORKOUT_ID>/exercises \
  -H "Authorization: Bearer <TOKEN>" \
  -H "Content-Type: application/json" \
  -d '{
    "exercise_id": "<EXERCISE_ID>",
    "sets": 4,
    "reps": 10,
    "weight": 80.0
  }'
```

### 6. Статистика за период

```bash
curl "http://localhost:8000/users/<USER_ID>/workouts/stats?start_date=2024-01-01&end_date=2024-12-31" \
  -H "Authorization: Bearer <TOKEN>"
```

## Тесты

```bash
pip install pytest httpx
pytest tests.py -v
```

## Структура проекта

```
homework_2/
├── main.py               # Основное приложение FastAPI
├── openapi.yaml          # OpenAPI спецификация
├── tests.py              # Unit-тесты
├── Dockerfile            # Образ контейнера
├── docker-compose.yaml   # Конфигурация Docker Compose
├── requirements.txt      # Зависимости Python
└── README.md             # Документация
```

## Модели данных

### User
| Поле | Тип | Описание |
|------|-----|----------|
| id | UUID | Идентификатор |
| username | string | Логин |
| first_name | string | Имя |
| last_name | string | Фамилия |
| email | string | Email |
| hashed_password | string | Хеш пароля |

### Exercise
| Поле | Тип | Описание |
|------|-----|----------|
| id | UUID | Идентификатор |
| name | string | Название |
| description | string | Описание |
| muscle_group | string | Группа мышц |

### Workout
| Поле | Тип | Описание |
|------|-----|----------|
| id | UUID | Идентификатор |
| user_id | UUID | ID пользователя |
| name | string | Название тренировки |
| date | string | Дата (YYYY-MM-DD) |
| exercises | list | Упражнения с подходами/повторениями |
