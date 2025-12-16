#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <cstring>
#include <libpq-fe.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <iomanip>
#include <ctime>

class HTTPServer {
private:
    int server_fd;
    struct sockaddr_in address;
    PGconn* conn;
    int port;

    std::string getCurrentTime() {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");
        return oss.str();
    }

    std::string urlDecode(const std::string& str) {
        std::string result;
        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '+') {
                result += ' ';
            } else if (str[i] == '%' && i + 2 < str.length()) {
                int value;
                std::istringstream is(str.substr(i + 1, 2));
                if (is >> std::hex >> value) {
                    result += static_cast<char>(value);
                    i += 2;
                } else {
                    result += str[i];
                }
            } else {
                result += str[i];
            }
        }
        return result;
    }

    std::map<std::string, std::string> parseQuery(const std::string& query) {
        std::map<std::string, std::string> params;
        std::istringstream iss(query);
        std::string pair;
        
        while (std::getline(iss, pair, '&')) {
            size_t pos = pair.find('=');
            if (pos != std::string::npos) {
                std::string key = urlDecode(pair.substr(0, pos));
                std::string value = urlDecode(pair.substr(pos + 1));
                params[key] = value;
            }
        }
        return params;
    }

    std::string readRequestBody(int client_fd, int contentLength) {
        std::string body;
        body.resize(contentLength);
        int totalRead = 0;
        while (totalRead < contentLength) {
            int n = read(client_fd, &body[totalRead], contentLength - totalRead);
            if (n <= 0) break;
            totalRead += n;
        }
        return body;
    }

    std::string sendResponse(int client_fd, const std::string& status, 
                            const std::string& contentType, const std::string& body) {
        std::ostringstream response;
        response << "HTTP/1.1 " << status << "\r\n";
        response << "Content-Type: " << contentType << "\r\n";
        response << "Content-Length: " << body.length() << "\r\n";
        response << "Date: " << getCurrentTime() << "\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
        response << "Access-Control-Allow-Headers: Content-Type\r\n";
        response << "\r\n";
        response << body;
        
        std::string responseStr = response.str();
        send(client_fd, responseStr.c_str(), responseStr.length(), 0);
        return responseStr;
    }

    std::string getAllShipments() {
        // Использование JOIN для получения данных из связанных таблиц
        const char* query = "SELECT s.id, s.cargo_description, s.origin, s.destination, s.weight_kg, s.volume_m3, s.status, s.created_at, s.updated_at, "
                            "COALESCE(c.name, '') as client_name, COALESCE(v.vehicle_type, '') as transport_type, COALESCE(v.license_plate, '') as vehicle_plate, "
                            "COALESCE(d.full_name, '') as driver_name "
                            "FROM shipments s "
                            "LEFT JOIN clients c ON s.client_id = c.id "
                            "LEFT JOIN vehicles v ON s.vehicle_id = v.id "
                            "LEFT JOIN drivers d ON s.driver_id = d.id "
                            "ORDER BY s.id";
        PGresult* res = PQexec(conn, query);
        
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::string error = PQerrorMessage(conn);
            PQclear(res);
            return "{\"error\":\"" + error + "\"}";
        }

        // Функция для экранирования JSON строк
        auto escapeJson = [](const std::string& str) -> std::string {
            std::string result;
            for (char c : str) {
                switch (c) {
                    case '"': result += "\\\""; break;
                    case '\\': result += "\\\\"; break;
                    case '\b': result += "\\b"; break;
                    case '\f': result += "\\f"; break;
                    case '\n': result += "\\n"; break;
                    case '\r': result += "\\r"; break;
                    case '\t': result += "\\t"; break;
                    default: result += c; break;
                }
            }
            return result;
        };

        std::ostringstream json;
        json << "{\"shipments\":[";
        
        int rows = PQntuples(res);
        for (int i = 0; i < rows; i++) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"id\":" << PQgetvalue(res, i, 0) << ",";
            json << "\"cargo_description\":\"" << escapeJson(PQgetvalue(res, i, 1) ? PQgetvalue(res, i, 1) : "") << "\",";
            json << "\"origin\":\"" << escapeJson(PQgetvalue(res, i, 2) ? PQgetvalue(res, i, 2) : "") << "\",";
            json << "\"destination\":\"" << escapeJson(PQgetvalue(res, i, 3) ? PQgetvalue(res, i, 3) : "") << "\",";
            json << "\"weight_kg\":" << PQgetvalue(res, i, 4) << ",";
            json << "\"volume_m3\":" << (PQgetvalue(res, i, 5) ? PQgetvalue(res, i, 5) : "null") << ",";
            json << "\"status\":\"" << escapeJson(PQgetvalue(res, i, 6) ? PQgetvalue(res, i, 6) : "") << "\",";
            json << "\"created_at\":\"" << escapeJson(PQgetvalue(res, i, 7) ? PQgetvalue(res, i, 7) : "") << "\",";
            json << "\"updated_at\":\"" << escapeJson(PQgetvalue(res, i, 8) ? PQgetvalue(res, i, 8) : "") << "\",";
            json << "\"client_name\":\"" << escapeJson(PQgetvalue(res, i, 9) ? PQgetvalue(res, i, 9) : "") << "\",";
            json << "\"transport_type\":\"" << escapeJson(PQgetvalue(res, i, 10) ? PQgetvalue(res, i, 10) : "") << "\",";
            json << "\"vehicle_plate\":\"" << escapeJson(PQgetvalue(res, i, 11) ? PQgetvalue(res, i, 11) : "") << "\",";
            json << "\"driver_name\":\"" << escapeJson(PQgetvalue(res, i, 12) ? PQgetvalue(res, i, 12) : "") << "\"";
            json << "}";
        }
        
        json << "]}";
        PQclear(res);
        return json.str();
    }

    std::string createShipment(const std::map<std::string, std::string>& params) {
        // Использование prepared statement для защиты от SQL инъекций
        // Поддержка как старого формата (transport_type), так и нового (vehicle_id)
        const char* query;
        const char* paramValues[10];
        int paramCount = 0;
        
        if (params.count("vehicle_id") && !params.at("vehicle_id").empty()) {
            // Новый формат с vehicle_id
            query = "INSERT INTO shipments (cargo_description, origin, destination, weight_kg, volume_m3, status, client_id, vehicle_id, driver_id) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) RETURNING id";
            paramValues[0] = params.at("cargo_description").c_str();
            paramValues[1] = params.at("origin").c_str();
            paramValues[2] = params.at("destination").c_str();
            paramValues[3] = params.at("weight_kg").c_str();
            paramValues[4] = params.count("volume_m3") && !params.at("volume_m3").empty() ? params.at("volume_m3").c_str() : nullptr;
            paramValues[5] = params.count("status") ? params.at("status").c_str() : "pending";
            paramValues[6] = params.count("client_id") && !params.at("client_id").empty() ? params.at("client_id").c_str() : nullptr;
            paramValues[7] = params.at("vehicle_id").c_str();
            paramValues[8] = params.count("driver_id") && !params.at("driver_id").empty() ? params.at("driver_id").c_str() : nullptr;
            paramCount = 9;
        } else {
            // Старый формат для обратной совместимости (без vehicle_id, используем transport_type как vehicle_type)
            query = "INSERT INTO shipments (cargo_description, origin, destination, weight_kg, volume_m3, status) VALUES ($1, $2, $3, $4, $5, $6) RETURNING id";
            paramValues[0] = params.at("cargo_description").c_str();
            paramValues[1] = params.at("origin").c_str();
            paramValues[2] = params.at("destination").c_str();
            paramValues[3] = params.at("weight_kg").c_str();
            paramValues[4] = params.count("volume_m3") && !params.at("volume_m3").empty() ? params.at("volume_m3").c_str() : nullptr;
            paramValues[5] = params.count("status") ? params.at("status").c_str() : "pending";
            paramCount = 6;
        }

        int paramLengths[10] = {0};
        int paramFormats[10] = {0};

        PGresult* res = PQexecParams(conn, query, paramCount, nullptr, paramValues, paramLengths, paramFormats, 0);
        
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::string error = PQerrorMessage(conn);
            PQclear(res);
            return "{\"error\":\"" + error + "\"}";
        }

        std::string id = PQgetvalue(res, 0, 0);
        PQclear(res);
        return "{\"success\":true,\"id\":" + id + "}";
    }

    std::string updateShipment(int id, const std::map<std::string, std::string>& params) {
        // Использование prepared statement для защиты от SQL инъекций
        // Поддержка как старого формата, так и нового с vehicle_id
        const char* query;
        const char* paramValues[10];
        int paramCount = 0;
        
        if (params.count("vehicle_id") && !params.at("vehicle_id").empty()) {
            // Новый формат с vehicle_id
            query = "UPDATE shipments SET cargo_description=$1, origin=$2, destination=$3, weight_kg=$4, volume_m3=$5, status=$6, client_id=$7, vehicle_id=$8, driver_id=$9 WHERE id=$10 RETURNING id";
            paramValues[0] = params.at("cargo_description").c_str();
            paramValues[1] = params.at("origin").c_str();
            paramValues[2] = params.at("destination").c_str();
            paramValues[3] = params.at("weight_kg").c_str();
            paramValues[4] = params.count("volume_m3") && !params.at("volume_m3").empty() ? params.at("volume_m3").c_str() : nullptr;
            paramValues[5] = params.at("status").c_str();
            paramValues[6] = params.count("client_id") && !params.at("client_id").empty() ? params.at("client_id").c_str() : nullptr;
            paramValues[7] = params.at("vehicle_id").c_str();
            paramValues[8] = params.count("driver_id") && !params.at("driver_id").empty() ? params.at("driver_id").c_str() : nullptr;
            paramValues[9] = std::to_string(id).c_str();
            paramCount = 10;
        } else {
            // Старый формат для обратной совместимости
            query = "UPDATE shipments SET cargo_description=$1, origin=$2, destination=$3, weight_kg=$4, volume_m3=$5, status=$6 WHERE id=$7 RETURNING id";
            paramValues[0] = params.at("cargo_description").c_str();
            paramValues[1] = params.at("origin").c_str();
            paramValues[2] = params.at("destination").c_str();
            paramValues[3] = params.at("weight_kg").c_str();
            paramValues[4] = params.count("volume_m3") && !params.at("volume_m3").empty() ? params.at("volume_m3").c_str() : nullptr;
            paramValues[5] = params.at("status").c_str();
            paramValues[6] = std::to_string(id).c_str();
            paramCount = 7;
        }

        int paramLengths[10] = {0};
        int paramFormats[10] = {0};

        PGresult* res = PQexecParams(conn, query, paramCount, nullptr, paramValues, paramLengths, paramFormats, 0);
        
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::string error = PQerrorMessage(conn);
            PQclear(res);
            return "{\"error\":\"" + error + "\"}";
        }

        if (PQntuples(res) == 0) {
            PQclear(res);
            return "{\"error\":\"Shipment not found\"}";
        }

        PQclear(res);
        return "{\"success\":true,\"id\":" + std::to_string(id) + "}";
    }

    std::string deleteShipment(int id) {
        // Использование prepared statement для защиты от SQL инъекций
        const char* query = "DELETE FROM shipments WHERE id=$1 RETURNING id";
        const char* paramValues[1];
        paramValues[0] = std::to_string(id).c_str();
        int paramLengths[1] = {0};
        int paramFormats[1] = {0};

        PGresult* res = PQexecParams(conn, query, 1, nullptr, paramValues, paramLengths, paramFormats, 0);
        
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::string error = PQerrorMessage(conn);
            PQclear(res);
            return "{\"error\":\"" + error + "\"}";
        }

        if (PQntuples(res) == 0) {
            PQclear(res);
            return "{\"error\":\"Shipment not found\"}";
        }

        PQclear(res);
        return "{\"success\":true}";
    }

    void handleRequest(int client_fd) {
        char buffer[4096] = {0};
        int valread = read(client_fd, buffer, 4096);
        
        if (valread <= 0) {
            close(client_fd);
            return;
        }

        std::string request(buffer, valread);
        std::istringstream iss(request);
        std::string method, path, version;
        iss >> method >> path >> version;

        // Обработка OPTIONS запроса для CORS
        if (method == "OPTIONS") {
            sendResponse(client_fd, "200 OK", "text/plain", "");
            close(client_fd);
            return;
        }

        // Парсинг пути
        size_t queryPos = path.find('?');
        std::string route = path.substr(0, queryPos);
        std::string query = queryPos != std::string::npos ? path.substr(queryPos + 1) : "";

        // Получение Content-Length из заголовков и поиск начала body
        int contentLength = 0;
        std::string line;
        size_t headerEndPos = request.find("\r\n\r\n");
        
        if (headerEndPos != std::string::npos) {
            std::string headers = request.substr(0, headerEndPos);
            std::istringstream headerStream(headers);
            
            while (std::getline(headerStream, line)) {
                if (line.find("Content-Length:") == 0 || line.find("Content-Length: ") == 0) {
                    size_t colonPos = line.find(':');
                    if (colonPos != std::string::npos) {
                        std::string lenStr = line.substr(colonPos + 1);
                        // Удаляем пробелы и \r если есть
                        while (!lenStr.empty() && (lenStr[0] == ' ' || lenStr[0] == '\t')) {
                            lenStr.erase(0, 1);
                        }
                        if (!lenStr.empty() && lenStr.back() == '\r') {
                            lenStr.pop_back();
                        }
                        if (!lenStr.empty()) {
                            contentLength = std::stoi(lenStr);
                        }
                    }
                }
            }
        }

        // Извлечение body из буфера
        std::string body;
        if (contentLength > 0 && headerEndPos != std::string::npos) {
            size_t bodyStart = headerEndPos + 4; // Пропускаем "\r\n\r\n"
            if (bodyStart + contentLength <= request.length()) {
                body = request.substr(bodyStart, contentLength);
            } else {
                // Если body не полностью в буфере, дочитываем из сокета
                body = request.substr(bodyStart);
                int remaining = contentLength - body.length();
                if (remaining > 0) {
                    std::string extra = readRequestBody(client_fd, remaining);
                    body += extra;
                }
            }
        }

        std::string responseBody;

        if (route == "/api/shipments" && method == "GET") {
            responseBody = getAllShipments();
            sendResponse(client_fd, "200 OK", "application/json", responseBody);
        }
        else if (route == "/api/shipments" && method == "POST") {
            auto params = parseQuery(body);
            
            // Проверка обязательных полей (поддержка старого и нового формата)
            bool hasRequiredFields = params.count("cargo_description") && params.count("origin") && 
                                    params.count("destination") && params.count("weight_kg") && 
                                    (params.count("transport_type") || params.count("vehicle_id"));
            
            if (hasRequiredFields) {
                responseBody = createShipment(params);
                sendResponse(client_fd, "201 Created", "application/json", responseBody);
            } else {
                responseBody = "{\"error\":\"Missing required fields\"}";
                sendResponse(client_fd, "400 Bad Request", "application/json", responseBody);
            }
        }
        else if (route.find("/api/shipments/") == 0 && method == "PUT") {
            int id = std::stoi(route.substr(15));
            auto params = parseQuery(body);
            
            // Проверка обязательных полей (поддержка старого и нового формата)
            bool hasRequiredFields = params.count("cargo_description") && params.count("origin") && 
                                    params.count("destination") && params.count("weight_kg") && 
                                    params.count("status") &&
                                    (params.count("transport_type") || params.count("vehicle_id"));
            
            if (hasRequiredFields) {
                responseBody = updateShipment(id, params);
                sendResponse(client_fd, "200 OK", "application/json", responseBody);
            } else {
                responseBody = "{\"error\":\"Missing required fields\"}";
                sendResponse(client_fd, "400 Bad Request", "application/json", responseBody);
            }
        }
        else if (route.find("/api/shipments/") == 0 && method == "DELETE") {
            int id = std::stoi(route.substr(15));
            responseBody = deleteShipment(id);
            sendResponse(client_fd, "200 OK", "application/json", responseBody);
        }
        else if (route == "/" || route == "/index.html") {
            std::ifstream file("../frontend/index.html");
            if (!file.is_open()) {
                file.open("frontend/index.html");
            }
            if (!file.is_open()) {
                file.open("/app/frontend/index.html");
            }
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                sendResponse(client_fd, "200 OK", "text/html", content);
            } else {
                sendResponse(client_fd, "404 Not Found", "text/plain", "File not found");
            }
        }
        else {
            sendResponse(client_fd, "404 Not Found", "text/plain", "Not Found");
        }

        close(client_fd);
    }

public:
    HTTPServer(int port) : port(port) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == 0) {
            std::cerr << "Socket creation failed" << std::endl;
            exit(1);
        }

        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
            std::cerr << "Setsockopt failed" << std::endl;
            exit(1);
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            std::cerr << "Bind failed" << std::endl;
            exit(1);
        }

        if (listen(server_fd, 10) < 0) {
            std::cerr << "Listen failed" << std::endl;
            exit(1);
        }

        // Подключение к PostgreSQL
        const char* conninfo = "host=postgres port=5432 dbname=logistics_db user=logistics_user password=logistics_pass";
        conn = PQconnectdb(conninfo);

        if (PQstatus(conn) != CONNECTION_OK) {
            std::cerr << "Connection to database failed: " << PQerrorMessage(conn) << std::endl;
            PQfinish(conn);
            exit(1);
        }

        std::cout << "Server started on port " << port << std::endl;
        std::cout << "Connected to PostgreSQL database" << std::endl;
    }

    void run() {
        while (true) {
            int addrlen = sizeof(address);
            int client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            
            if (client_fd < 0) {
                continue;
            }

            std::thread(&HTTPServer::handleRequest, this, client_fd).detach();
        }
    }

    ~HTTPServer() {
        close(server_fd);
        PQfinish(conn);
    }
};

int main() {
    HTTPServer server(8080);
    server.run();
    return 0;
}

