.PHONY: help build up down restart logs clean rebuild test db-reset

# Переменные
COMPOSE = docker-compose
COMPOSE_FILE = docker-compose.yml
BACKEND_SERVICE = backend
POSTGRES_SERVICE = postgres

# Цвета для вывода
GREEN = \033[0;32m
YELLOW = \033[0;33m
NC = \033[0m # No Color

help: ## Показать справку по командам
	@echo "$(GREEN)Доступные команды:$(NC)"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "  $(YELLOW)%-15s$(NC) %s\n", $$1, $$2}'

build: ## Собрать Docker образы
	@echo "$(GREEN)Сборка Docker образов...$(NC)"
	$(COMPOSE) build

build-no-cache: ## Собрать Docker образы без кеша
	@echo "$(GREEN)Сборка Docker образов без кеша...$(NC)"
	$(COMPOSE) build --no-cache

up: ## Запустить контейнеры
	@echo "$(GREEN)Запуск контейнеров...$(NC)"
	$(COMPOSE) up -d

up-build: ## Собрать и запустить контейнеры
	@echo "$(GREEN)Сборка и запуск контейнеров...$(NC)"
	$(COMPOSE) up -d --build

down: ## Остановить контейнеры
	@echo "$(GREEN)Остановка контейнеров...$(NC)"
	$(COMPOSE) down

down-volumes: ## Остановить контейнеры и удалить volumes
	@echo "$(GREEN)Остановка контейнеров и удаление volumes...$(NC)"
	$(COMPOSE) down -v

restart: ## Перезапустить контейнеры
	@echo "$(GREEN)Перезапуск контейнеров...$(NC)"
	$(COMPOSE) restart

restart-backend: ## Перезапустить только backend
	@echo "$(GREEN)Перезапуск backend...$(NC)"
	$(COMPOSE) restart $(BACKEND_SERVICE)

rebuild: ## Пересобрать и перезапустить контейнеры
	@echo "$(GREEN)Пересборка и перезапуск контейнеров...$(NC)"
	$(COMPOSE) up -d --build --force-recreate

rebuild-backend: ## Пересобрать и перезапустить только backend
	@echo "$(GREEN)Пересборка и перезапуск backend...$(NC)"
	$(COMPOSE) build $(BACKEND_SERVICE)
	$(COMPOSE) up -d --force-recreate $(BACKEND_SERVICE)

logs: ## Показать логи всех сервисов
	$(COMPOSE) logs -f

logs-backend: ## Показать логи backend
	$(COMPOSE) logs -f $(BACKEND_SERVICE)

logs-postgres: ## Показать логи PostgreSQL
	$(COMPOSE) logs -f $(POSTGRES_SERVICE)

ps: ## Показать статус контейнеров
	$(COMPOSE) ps

status: ps ## Показать статус контейнеров (алиас)

clean: ## Остановить и удалить контейнеры, сети
	@echo "$(GREEN)Очистка контейнеров и сетей...$(NC)"
	$(COMPOSE) down --remove-orphans

clean-all: down-volumes ## Полная очистка (контейнеры, сети, volumes)
	@echo "$(GREEN)Полная очистка...$(NC)"
	$(COMPOSE) down -v --remove-orphans
	@echo "$(YELLOW)Внимание: Все данные в базе будут удалены!$(NC)"

db-reset: ## Сбросить базу данных (удалить volumes и пересоздать)
	@echo "$(YELLOW)Сброс базы данных...$(NC)"
	$(COMPOSE) down -v
	$(COMPOSE) up -d
	@echo "$(GREEN)База данных пересоздана$(NC)"

db-connect: ## Подключиться к PostgreSQL через psql
	@echo "$(GREEN)Подключение к PostgreSQL...$(NC)"
	$(COMPOSE) exec $(POSTGRES_SERVICE) psql -U logistics_user -d logistics_db

db-shell: db-connect ## Подключиться к PostgreSQL (алиас)

test: ## Запустить тесты API
	@echo "$(GREEN)Тестирование API...$(NC)"
	@echo "GET /api/shipments:"
	@curl -s http://localhost:8080/api/shipments | head -c 200
	@echo "\n"
	@echo "$(GREEN)Тест завершен$(NC)"

test-post: ## Тест POST запроса
	@echo "$(GREEN)Тест POST запроса...$(NC)"
	@curl -X POST http://localhost:8080/api/shipments \
		-H "Content-Type: application/x-www-form-urlencoded" \
		-d "cargo_description=Тест&origin=Москва&destination=СПб&weight_kg=100&transport_type=Грузовик&status=pending"
	@echo "\n"

shell-backend: ## Открыть shell в контейнере backend
	$(COMPOSE) exec $(BACKEND_SERVICE) /bin/bash

shell-postgres: ## Открыть shell в контейнере PostgreSQL
	$(COMPOSE) exec $(POSTGRES_SERVICE) /bin/sh

dev: up logs ## Запустить в режиме разработки (с логами)

stop: down ## Остановить контейнеры (алиас)

start: up ## Запустить контейнеры (алиас)

.DEFAULT_GOAL := help

