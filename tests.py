import pytest
from fastapi.testclient import TestClient
from main import app, users_db, exercises_db, workouts_db, user_tokens_db


@pytest.fixture(autouse=True)
def clear_db():
    users_db.clear()
    exercises_db.clear()
    workouts_db.clear()
    user_tokens_db.clear()


@pytest.fixture
def client():
    return TestClient(app)


@pytest.fixture
def auth_client(client):
    client.post("/register", json={
        "username": "testuser",
        "first_name": "Test",
        "last_name": "User",
        "email": "test@example.com",
        "password": "password123",
    })
    resp = client.post("/token", data={"username": "testuser", "password": "password123"})
    token = resp.json()["access_token"]
    return client, token


# ==================== Auth tests ====================

class TestAuth:
    def test_register_success(self, client):
        resp = client.post("/register", json={
            "username": "newuser",
            "first_name": "New",
            "last_name": "User",
            "email": "new@example.com",
            "password": "pass1234",
        })
        assert resp.status_code == 201
        data = resp.json()
        assert data["username"] == "newuser"
        assert data["first_name"] == "New"
        assert "id" in data

    def test_register_duplicate_username(self, client):
        payload = {
            "username": "dupuser",
            "first_name": "Dup",
            "last_name": "User",
            "email": "dup@example.com",
            "password": "pass1234",
        }
        client.post("/register", json=payload)
        resp = client.post("/register", json={**payload, "email": "dup2@example.com"})
        assert resp.status_code == 409

    def test_login_success(self, client):
        client.post("/register", json={
            "username": "loginuser",
            "first_name": "Login",
            "last_name": "User",
            "email": "login@example.com",
            "password": "secret123",
        })
        resp = client.post("/token", data={"username": "loginuser", "password": "secret123"})
        assert resp.status_code == 200
        assert "access_token" in resp.json()

    def test_login_wrong_password(self, client):
        client.post("/register", json={
            "username": "wronguser",
            "first_name": "Wrong",
            "last_name": "User",
            "email": "wrong@example.com",
            "password": "correct",
        })
        resp = client.post("/token", data={"username": "wronguser", "password": "incorrect"})
        assert resp.status_code == 401

    def test_login_nonexistent_user(self, client):
        resp = client.post("/token", data={"username": "nobody", "password": "none"})
        assert resp.status_code == 401


# ==================== User tests ====================

class TestUsers:
    def test_create_user(self, client):
        resp = client.post("/users", json={
            "username": "alice",
            "first_name": "Alice",
            "last_name": "Smith",
            "email": "alice@example.com",
            "password": "pass1234",
        })
        assert resp.status_code == 201
        assert resp.json()["username"] == "alice"

    def test_search_by_username(self, client):
        client.post("/users", json={
            "username": "bob",
            "first_name": "Bob",
            "last_name": "Jones",
            "email": "bob@example.com",
            "password": "pass1234",
        })
        resp = client.get("/users/search?username=bob")
        assert resp.status_code == 200
        assert len(resp.json()) == 1
        assert resp.json()[0]["username"] == "bob"

    def test_search_by_name_mask(self, client):
        client.post("/users", json={
            "username": "john",
            "first_name": "John",
            "last_name": "Doe",
            "email": "john@example.com",
            "password": "pass1234",
        })
        resp = client.get("/users/search?first_name=Jo*&last_name=D*")
        assert resp.status_code == 200
        assert len(resp.json()) == 1

    def test_search_no_params(self, client):
        resp = client.get("/users/search")
        assert resp.status_code == 400

    def test_search_not_found(self, client):
        resp = client.get("/users/search?username=nonexistent")
        assert resp.status_code == 200
        assert len(resp.json()) == 0


# ==================== Exercise tests ====================

class TestExercises:
    def test_create_exercise(self, auth_client):
        client, token = auth_client
        resp = client.post("/exercises", json={
            "name": "Приседания",
            "description": "Базовое упражнение для ног",
            "muscle_group": "Ноги",
        }, headers={"Authorization": f"Bearer {token}"})
        assert resp.status_code == 201
        assert resp.json()["name"] == "Приседания"

    def test_create_exercise_unauthorized(self, client):
        resp = client.post("/exercises", json={
            "name": "Приседания",
            "description": "Базовое упражнение для ног",
            "muscle_group": "Ноги",
        })
        assert resp.status_code == 401

    def test_get_exercises(self, client):
        resp = client.get("/exercises")
        assert resp.status_code == 200
        assert isinstance(resp.json(), list)


# ==================== Workout tests ====================

class TestWorkouts:
    def _create_user_and_exercise(self, client, token):
        user_resp = client.post("/users", json={
            "username": "athlete",
            "first_name": "Ath",
            "last_name": "Lete",
            "email": "ath@example.com",
            "password": "pass1234",
        })
        user_id = user_resp.json()["id"]

        ex_resp = client.post("/exercises", json={
            "name": "Становая тяга",
            "description": "Тяга",
            "muscle_group": "Спина",
        }, headers={"Authorization": f"Bearer {token}"})
        exercise_id = ex_resp.json()["id"]
        return user_id, exercise_id

    def test_create_workout(self, auth_client):
        client, token = auth_client
        user_id, _ = self._create_user_and_exercise(client, token)

        resp = client.post(f"/users/{user_id}/workouts", json={
            "name": "Тренировка спины",
            "date": "2024-03-20",
        }, headers={"Authorization": f"Bearer {token}"})
        assert resp.status_code == 201
        assert resp.json()["name"] == "Тренировка спины"
        assert resp.json()["user_id"] == user_id

    def test_add_exercise_to_workout(self, auth_client):
        client, token = auth_client
        user_id, exercise_id = self._create_user_and_exercise(client, token)

        workout_resp = client.post(f"/users/{user_id}/workouts", json={
            "name": "Силовая",
            "date": "2024-03-20",
        }, headers={"Authorization": f"Bearer {token}"})
        workout_id = workout_resp.json()["id"]

        resp = client.post(
            f"/users/{user_id}/workouts/{workout_id}/exercises",
            json={"exercise_id": exercise_id, "sets": 3, "reps": 8, "weight": 100.0},
            headers={"Authorization": f"Bearer {token}"},
        )
        assert resp.status_code == 200
        assert len(resp.json()["exercises"]) == 1
        assert resp.json()["exercises"][0]["sets"] == 3

    def test_get_user_workouts(self, auth_client):
        client, token = auth_client
        user_id, _ = self._create_user_and_exercise(client, token)

        client.post(f"/users/{user_id}/workouts", json={
            "name": "Тренировка 1",
            "date": "2024-03-20",
        }, headers={"Authorization": f"Bearer {token}"})
        client.post(f"/users/{user_id}/workouts", json={
            "name": "Тренировка 2",
            "date": "2024-03-21",
        }, headers={"Authorization": f"Bearer {token}"})

        resp = client.get(f"/users/{user_id}/workouts",
                          headers={"Authorization": f"Bearer {token}"})
        assert resp.status_code == 200
        assert len(resp.json()) == 2

    def test_workout_stats(self, auth_client):
        client, token = auth_client
        user_id, exercise_id = self._create_user_and_exercise(client, token)

        for i in range(3):
            workout_resp = client.post(f"/users/{user_id}/workouts", json={
                "name": f"Workout {i}",
                "date": f"2024-03-{10 + i:02d}",
            }, headers={"Authorization": f"Bearer {token}"})
            workout_id = workout_resp.json()["id"]
            client.post(
                f"/users/{user_id}/workouts/{workout_id}/exercises",
                json={"exercise_id": exercise_id, "sets": 4, "reps": 10, "weight": 50.0},
                headers={"Authorization": f"Bearer {token}"},
            )

        resp = client.get(
            f"/users/{user_id}/workouts/stats?start_date=2024-03-01&end_date=2024-03-31",
            headers={"Authorization": f"Bearer {token}"},
        )
        assert resp.status_code == 200
        stats = resp.json()
        assert stats["total_workouts"] == 3
        assert stats["total_exercises"] == 3
        assert stats["total_sets"] == 12
        assert stats["total_reps"] == 120

    def test_create_workout_unauthorized(self, client):
        resp = client.post("/users/fake-id/workouts", json={
            "name": "Test",
            "date": "2024-01-01",
        })
        assert resp.status_code == 401

    def test_workout_user_not_found(self, auth_client):
        client, token = auth_client
        resp = client.post("/users/nonexistent-id/workouts", json={
            "name": "Test",
            "date": "2024-01-01",
        }, headers={"Authorization": f"Bearer {token}"})
        assert resp.status_code == 404

    def test_add_exercise_to_nonexistent_workout(self, auth_client):
        client, token = auth_client
        user_resp = client.post("/users", json={
            "username": "test2",
            "first_name": "T",
            "last_name": "T",
            "email": "t@t.com",
            "password": "pass1234",
        })
        user_id = user_resp.json()["id"]

        resp = client.post(
            f"/users/{user_id}/workouts/fake-workout/exercises",
            json={"exercise_id": "fake-ex", "sets": 1, "reps": 1},
            headers={"Authorization": f"Bearer {token}"},
        )
        assert resp.status_code == 404
