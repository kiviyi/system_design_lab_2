from fastapi import FastAPI, HTTPException, Depends, status, Query
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from pydantic import BaseModel, Field
from typing import List, Optional
from datetime import datetime, timedelta
from jose import JWTError, jwt
from passlib.context import CryptContext
import uuid
import fnmatch

SECRET_KEY = "fitness-tracker-secret-key-2024"
ALGORITHM = "HS256"
ACCESS_TOKEN_EXPIRE_MINUTES = 60

app = FastAPI(
    title="Fitness Tracker API",
    description="REST API для фитнес-трекера: управление пользователями, упражнениями и тренировками",
    version="1.0.0",
)

pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")
oauth2_scheme = OAuth2PasswordBearer(tokenUrl="token")


# ==================== DTO ====================

class UserCreate(BaseModel):
    username: str = Field(..., min_length=3, max_length=50)
    first_name: str = Field(..., min_length=1, max_length=100)
    last_name: str = Field(..., min_length=1, max_length=100)
    email: str = Field(..., pattern=r"^[\w\.-]+@[\w\.-]+\.\w+$")
    password: str = Field(..., min_length=6)


class UserResponse(BaseModel):
    id: str
    username: str
    first_name: str
    last_name: str
    email: str


class ExerciseCreate(BaseModel):
    name: str = Field(..., min_length=1, max_length=200)
    description: str = Field(default="", max_length=1000)
    muscle_group: str = Field(..., min_length=1, max_length=100)


class ExerciseResponse(BaseModel):
    id: str
    name: str
    description: str
    muscle_group: str


class WorkoutExerciseAdd(BaseModel):
    exercise_id: str
    sets: int = Field(..., ge=1, le=100)
    reps: int = Field(..., ge=1, le=1000)
    weight: float = Field(default=0, ge=0)


class WorkoutExerciseResponse(BaseModel):
    exercise_id: str
    exercise_name: str
    sets: int
    reps: int
    weight: float


class WorkoutCreate(BaseModel):
    name: str = Field(..., min_length=1, max_length=200)
    date: Optional[str] = None


class WorkoutResponse(BaseModel):
    id: str
    user_id: str
    name: str
    date: str
    exercises: List[WorkoutExerciseResponse]


class WorkoutStats(BaseModel):
    user_id: str
    start_date: str
    end_date: str
    total_workouts: int
    total_exercises: int
    total_sets: int
    total_reps: int
    workouts: List[WorkoutResponse]


class Token(BaseModel):
    access_token: str
    token_type: str


# ==================== In-memory storage ====================

users_db: dict = {}
exercises_db: dict = {}
workouts_db: dict = {}
user_tokens_db: dict = {}  # username -> hashed_password


# ==================== Auth helpers ====================

def create_access_token(data: dict, expires_delta: Optional[timedelta] = None):
    to_encode = data.copy()
    expire = datetime.utcnow() + (expires_delta or timedelta(minutes=15))
    to_encode.update({"exp": expire})
    return jwt.encode(to_encode, SECRET_KEY, algorithm=ALGORITHM)


async def get_current_user(token: str = Depends(oauth2_scheme)):
    credentials_exception = HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Could not validate credentials",
        headers={"WWW-Authenticate": "Bearer"},
    )
    try:
        payload = jwt.decode(token, SECRET_KEY, algorithms=[ALGORITHM])
        username = payload.get("sub")
        if username is None or username not in user_tokens_db:
            raise credentials_exception
        return str(username)
    except JWTError:
        raise credentials_exception


# ==================== Auth endpoints ====================

@app.post("/register", response_model=UserResponse, status_code=status.HTTP_201_CREATED,
          tags=["Auth"], summary="Регистрация нового пользователя")
def register(user_data: UserCreate):
    for u in users_db.values():
        if u["username"] == user_data.username:
            raise HTTPException(status_code=409, detail="Username already exists")
        if u["email"] == user_data.email:
            raise HTTPException(status_code=409, detail="Email already exists")

    user_id = str(uuid.uuid4())
    hashed_password = pwd_context.hash(user_data.password)

    user = {
        "id": user_id,
        "username": user_data.username,
        "first_name": user_data.first_name,
        "last_name": user_data.last_name,
        "email": user_data.email,
        "hashed_password": hashed_password,
    }
    users_db[user_id] = user
    user_tokens_db[user_data.username] = hashed_password

    return UserResponse(**{k: user[k] for k in UserResponse.model_fields})


@app.post("/token", response_model=Token, tags=["Auth"], summary="Получение JWT токена (логин)")
def login(form_data: OAuth2PasswordRequestForm = Depends()):
    if form_data.username not in user_tokens_db:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Incorrect username or password",
            headers={"WWW-Authenticate": "Bearer"},
        )

    hashed_password = user_tokens_db[form_data.username]
    if not pwd_context.verify(form_data.password, hashed_password):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Incorrect username or password",
            headers={"WWW-Authenticate": "Bearer"},
        )

    access_token = create_access_token(
        data={"sub": form_data.username},
        expires_delta=timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES),
    )
    return {"access_token": access_token, "token_type": "bearer"}


# ==================== User endpoints ====================

@app.post("/users", response_model=UserResponse, status_code=status.HTTP_201_CREATED,
          tags=["Users"], summary="Создание нового пользователя")
def create_user(user_data: UserCreate):
    for u in users_db.values():
        if u["username"] == user_data.username:
            raise HTTPException(status_code=409, detail="Username already exists")
        if u["email"] == user_data.email:
            raise HTTPException(status_code=409, detail="Email already exists")

    user_id = str(uuid.uuid4())
    hashed_password = pwd_context.hash(user_data.password)

    user = {
        "id": user_id,
        "username": user_data.username,
        "first_name": user_data.first_name,
        "last_name": user_data.last_name,
        "email": user_data.email,
        "hashed_password": hashed_password,
    }
    users_db[user_id] = user
    user_tokens_db[user_data.username] = hashed_password

    return UserResponse(**{k: user[k] for k in UserResponse.model_fields})


@app.get("/users/search", response_model=List[UserResponse], tags=["Users"],
         summary="Поиск пользователей по логину или маске имени/фамилии")
def search_users(
    username: Optional[str] = Query(None, description="Поиск по логину (exact)"),
    first_name: Optional[str] = Query(None, description="Маска имени (* для wildcard)"),
    last_name: Optional[str] = Query(None, description="Маска фамилии (* для wildcard)"),
):
    results = []
    for user in users_db.values():
        if username and user["username"] == username:
            results.append(user)
        elif first_name and last_name:
            if fnmatch.fnmatch(user["first_name"].lower(), first_name.lower()) and \
               fnmatch.fnmatch(user["last_name"].lower(), last_name.lower()):
                results.append(user)
        elif first_name and fnmatch.fnmatch(user["first_name"].lower(), first_name.lower()):
            results.append(user)
        elif last_name and fnmatch.fnmatch(user["last_name"].lower(), last_name.lower()):
            results.append(user)

    if not username and not first_name and not last_name:
        raise HTTPException(status_code=400, detail="Provide at least one search parameter")

    return [UserResponse(**{k: u[k] for k in UserResponse.model_fields}) for u in results]


# ==================== Exercise endpoints ====================

@app.post("/exercises", response_model=ExerciseResponse, status_code=status.HTTP_201_CREATED,
          tags=["Exercises"], summary="Создание упражнения")
def create_exercise(exercise_data: ExerciseCreate, current_user: str = Depends(get_current_user)):
    exercise_id = str(uuid.uuid4())
    exercise = {
        "id": exercise_id,
        "name": exercise_data.name,
        "description": exercise_data.description,
        "muscle_group": exercise_data.muscle_group,
    }
    exercises_db[exercise_id] = exercise
    return ExerciseResponse(**exercise)


@app.get("/exercises", response_model=List[ExerciseResponse], tags=["Exercises"],
         summary="Получение списка упражнений")
def get_exercises():
    return [ExerciseResponse(**e) for e in exercises_db.values()]


# ==================== Workout endpoints ====================

@app.post("/users/{user_id}/workouts", response_model=WorkoutResponse, status_code=status.HTTP_201_CREATED,
          tags=["Workouts"], summary="Создание тренировки")
def create_workout(user_id: str, workout_data: WorkoutCreate, current_user: str = Depends(get_current_user)):
    if user_id not in users_db:
        raise HTTPException(status_code=404, detail="User not found")

    workout_id = str(uuid.uuid4())
    workout_date = workout_data.date or datetime.utcnow().strftime("%Y-%m-%d")

    workout = {
        "id": workout_id,
        "user_id": user_id,
        "name": workout_data.name,
        "date": workout_date,
        "exercises": [],
    }
    workouts_db[workout_id] = workout
    return WorkoutResponse(
        id=workout["id"],
        user_id=workout["user_id"],
        name=workout["name"],
        date=workout["date"],
        exercises=[],
    )


@app.post("/users/{user_id}/workouts/{workout_id}/exercises", response_model=WorkoutResponse,
          tags=["Workouts"], summary="Добавление упражнения в тренировку")
def add_exercise_to_workout(
    user_id: str, workout_id: str,
    exercise_data: WorkoutExerciseAdd,
    current_user: str = Depends(get_current_user),
):
    if user_id not in users_db:
        raise HTTPException(status_code=404, detail="User not found")
    if workout_id not in workouts_db:
        raise HTTPException(status_code=404, detail="Workout not found")
    if exercises_db.get(exercise_data.exercise_id) is None:
        raise HTTPException(status_code=404, detail="Exercise not found")

    workout = workouts_db[workout_id]
    if workout["user_id"] != user_id:
        raise HTTPException(status_code=403, detail="Workout does not belong to this user")

    exercise = exercises_db[exercise_data.exercise_id]
    workout_exercise = {
        "exercise_id": exercise_data.exercise_id,
        "exercise_name": exercise["name"],
        "sets": exercise_data.sets,
        "reps": exercise_data.reps,
        "weight": exercise_data.weight,
    }
    workout["exercises"].append(workout_exercise)

    return WorkoutResponse(
        id=workout["id"],
        user_id=workout["user_id"],
        name=workout["name"],
        date=workout["date"],
        exercises=[WorkoutExerciseResponse(**we) for we in workout["exercises"]],
    )


@app.get("/users/{user_id}/workouts", response_model=List[WorkoutResponse], tags=["Workouts"],
         summary="Получение истории тренировок пользователя")
def get_user_workouts(user_id: str, current_user: str = Depends(get_current_user)):
    if user_id not in users_db:
        raise HTTPException(status_code=404, detail="User not found")

    user_workouts = [
        WorkoutResponse(
            id=w["id"],
            user_id=w["user_id"],
            name=w["name"],
            date=w["date"],
            exercises=[WorkoutExerciseResponse(**we) for we in w["exercises"]],
        )
        for w in workouts_db.values() if w["user_id"] == user_id
    ]
    user_workouts.sort(key=lambda x: x.date, reverse=True)
    return user_workouts


@app.get("/users/{user_id}/workouts/stats", response_model=WorkoutStats, tags=["Workouts"],
         summary="Получение статистики тренировок за период")
def get_workout_stats(
    user_id: str,
    start_date: str = Query(..., description="Начальная дата (YYYY-MM-DD)"),
    end_date: str = Query(..., description="Конечная дата (YYYY-MM-DD)"),
    current_user: str = Depends(get_current_user),
):
    if user_id not in users_db:
        raise HTTPException(status_code=404, detail="User not found")

    user_workouts = [
        w for w in workouts_db.values()
        if w["user_id"] == user_id and start_date <= w["date"] <= end_date
    ]

    total_exercises = sum(len(w["exercises"]) for w in user_workouts)
    total_sets = sum(we["sets"] for w in user_workouts for we in w["exercises"])
    total_reps = sum(we["reps"] * we["sets"] for w in user_workouts for we in w["exercises"])

    return WorkoutStats(
        user_id=user_id,
        start_date=start_date,
        end_date=end_date,
        total_workouts=len(user_workouts),
        total_exercises=total_exercises,
        total_sets=total_sets,
        total_reps=total_reps,
        workouts=[
            WorkoutResponse(
                id=w["id"],
                user_id=w["user_id"],
                name=w["name"],
                date=w["date"],
                exercises=[WorkoutExerciseResponse(**we) for we in w["exercises"]],
            )
            for w in user_workouts
        ],
    )


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
