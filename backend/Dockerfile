FROM ubuntu:22.04

# Установка зависимостей
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    libpq-dev \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Копирование исходного кода бэкенда
COPY backend/ .

# Копирование фронтенда
COPY frontend /app/frontend

# Компиляция с правильными путями для libpq
RUN g++ -std=c++17 -I/usr/include/postgresql -o server main.cpp -lpq -pthread

# Запуск сервера
CMD ["./server"]

