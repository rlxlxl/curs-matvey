-- Создание таблицы клиентов (заказчиков)
CREATE TABLE IF NOT EXISTS clients (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    contact_person VARCHAR(255),
    phone VARCHAR(50),
    email VARCHAR(255),
    address TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Создание таблицы транспортных средств
CREATE TABLE IF NOT EXISTS vehicles (
    id SERIAL PRIMARY KEY,
    license_plate VARCHAR(20) UNIQUE NOT NULL,
    vehicle_type VARCHAR(50) NOT NULL,
    capacity_kg DECIMAL(10, 2) NOT NULL,
    volume_m3 DECIMAL(10, 2),
    status VARCHAR(50) NOT NULL DEFAULT 'available',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Создание таблицы водителей
CREATE TABLE IF NOT EXISTS drivers (
    id SERIAL PRIMARY KEY,
    full_name VARCHAR(255) NOT NULL,
    license_number VARCHAR(50) UNIQUE NOT NULL,
    phone VARCHAR(50),
    email VARCHAR(255),
    status VARCHAR(50) NOT NULL DEFAULT 'available',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Создание таблицы для логистических перевозок с внешними ключами
CREATE TABLE IF NOT EXISTS shipments (
    id SERIAL PRIMARY KEY,
    client_id INTEGER REFERENCES clients(id) ON DELETE SET NULL,
    vehicle_id INTEGER REFERENCES vehicles(id) ON DELETE SET NULL,
    driver_id INTEGER REFERENCES drivers(id) ON DELETE SET NULL,
    cargo_description VARCHAR(255) NOT NULL,
    origin VARCHAR(255) NOT NULL,
    destination VARCHAR(255) NOT NULL,
    weight_kg DECIMAL(10, 2) NOT NULL,
    volume_m3 DECIMAL(10, 2),
    status VARCHAR(50) NOT NULL DEFAULT 'pending',
    departure_date TIMESTAMP,
    arrival_date TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT check_weight CHECK (weight_kg > 0),
    CONSTRAINT check_volume CHECK (volume_m3 IS NULL OR volume_m3 > 0)
);

-- Создание таблицы для администраторов
CREATE TABLE IF NOT EXISTS admins (
    id SERIAL PRIMARY KEY,
    username VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Вставка тестовых данных клиентов
INSERT INTO clients (name, contact_person, phone, email, address) VALUES
('ООО "Торговый Дом"', 'Иванов Иван Иванович', '+7 (495) 123-45-67', 'ivanov@td.ru', 'Москва, ул. Ленина, д. 1'),
('ИП Петров', 'Петров Петр Петрович', '+7 (812) 234-56-78', 'petrov@mail.ru', 'Санкт-Петербург, пр. Невский, д. 10'),
('ЗАО "СтройМатериалы"', 'Сидоров Сидор Сидорович', '+7 (343) 345-67-89', 'sidorov@stroy.ru', 'Екатеринбург, ул. Мира, д. 5'),
('ООО "Продукты+"', 'Козлова Мария Сергеевна', '+7 (383) 456-78-90', 'kozlova@prod.ru', 'Новосибирск, ул. Красный проспект, д. 20');

-- Вставка тестовых данных транспортных средств
INSERT INTO vehicles (license_plate, vehicle_type, capacity_kg, volume_m3, status) VALUES
('А123БВ777', 'Грузовик', 5000.00, 25.0, 'available'),
('В456ГД777', 'Фура', 20000.00, 80.0, 'available'),
('С789ЕЖ777', 'Рефрижератор', 10000.00, 40.0, 'available'),
('Д012ЗИ777', 'Грузовик', 3500.00, 15.0, 'in_use'),
('Е345КЛ777', 'Фура', 25000.00, 100.0, 'available');

-- Вставка тестовых данных водителей
INSERT INTO drivers (full_name, license_number, phone, email, status) VALUES
('Смирнов Алексей Викторович', '77АВ123456', '+7 (495) 111-22-33', 'smirnov@mail.ru', 'available'),
('Кузнецов Дмитрий Сергеевич', '78ВГ234567', '+7 (812) 222-33-44', 'kuznetsov@mail.ru', 'available'),
('Попов Андрей Николаевич', '66СД345678', '+7 (343) 333-44-55', 'popov@mail.ru', 'in_use'),
('Васильев Сергей Петрович', '54НС456789', '+7 (383) 444-55-66', 'vasiliev@mail.ru', 'available'),
('Новиков Игорь Александрович', '23КР567890', '+7 (861) 555-66-77', 'novikov@mail.ru', 'available');

-- Вставка тестовых данных перевозок с связями
INSERT INTO shipments (client_id, vehicle_id, driver_id, cargo_description, origin, destination, weight_kg, volume_m3, status, departure_date) VALUES
(1, 1, 1, 'Электроника', 'Москва', 'Санкт-Петербург', 500.00, 2.5, 'в_пути', CURRENT_TIMESTAMP - INTERVAL '2 days'),
(2, 2, 2, 'Мебель', 'Казань', 'Екатеринбург', 1200.00, 8.0, 'ожидает', NULL),
(3, 3, 3, 'Продукты питания', 'Новосибирск', 'Красноярск', 800.00, 5.0, 'доставлено', CURRENT_TIMESTAMP - INTERVAL '5 days'),
(4, 4, 4, 'Строительные материалы', 'Ростов-на-Дону', 'Воронеж', 3000.00, 15.0, 'в_пути', CURRENT_TIMESTAMP - INTERVAL '1 day'),
(1, 5, 5, 'Одежда', 'Сочи', 'Краснодар', 200.00, 1.5, 'ожидает', NULL);

-- Создание индексов для оптимизации запросов
CREATE INDEX idx_shipments_status ON shipments(status);
CREATE INDEX idx_shipments_origin ON shipments(origin);
CREATE INDEX idx_shipments_destination ON shipments(destination);
CREATE INDEX idx_shipments_client_id ON shipments(client_id);
CREATE INDEX idx_shipments_vehicle_id ON shipments(vehicle_id);
CREATE INDEX idx_shipments_driver_id ON shipments(driver_id);
CREATE INDEX idx_shipments_created_at ON shipments(created_at);
CREATE INDEX idx_clients_name ON clients(name);
CREATE INDEX idx_vehicles_license_plate ON vehicles(license_plate);
CREATE INDEX idx_drivers_license_number ON drivers(license_number);

-- Функция для автоматического обновления updated_at
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Триггер для автоматического обновления updated_at
CREATE TRIGGER update_shipments_updated_at BEFORE UPDATE ON shipments
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

-- Представление для удобного просмотра перевозок с данными связанных таблиц
CREATE OR REPLACE VIEW shipments_full AS
SELECT 
    s.id,
    s.cargo_description,
    s.origin,
    s.destination,
    s.weight_kg,
    s.volume_m3,
    s.status,
    s.departure_date,
    s.arrival_date,
    s.created_at,
    s.updated_at,
    c.name AS client_name,
    c.contact_person AS client_contact,
    c.phone AS client_phone,
    v.license_plate AS vehicle_plate,
    v.vehicle_type,
    v.capacity_kg AS vehicle_capacity,
    d.full_name AS driver_name,
    d.phone AS driver_phone,
    d.license_number AS driver_license
FROM shipments s
LEFT JOIN clients c ON s.client_id = c.id
LEFT JOIN vehicles v ON s.vehicle_id = v.id
LEFT JOIN drivers d ON s.driver_id = d.id;
