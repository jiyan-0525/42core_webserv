// minimal.cpp
//
// НАЙПРОСТІШИЙ можливий сервер. Жодних класів, жодних перевірок помилок,
// жодного poll(), жодного конфігу. Тільки голий мінімум, щоб зрозуміти,
// що взагалі відбувається. Обробляє ОДНОГО клієнта і завершується.
//
// Компіляція: g++ -std=c++17 minimal.cpp -o minimal
// Запуск:     ./minimal
// Перевірка:  curl http://localhost:8080/     (в іншому терміналі)

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main()
{
    // --- КРОК 1: створити сокет ---
    // "socket" - це як попросити ОС видати нам порожню телефонну трубку.
    // Повертає число (файловий дескриптор) - наш "номер" цієї трубки.
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // --- КРОК 2: прив'язати сокет до порту 8080 ---
    // Кажемо ОС: "ця трубка буде відповідати на порту 8080"
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;   // слухати на всіх мережевих інтерфейсах
    address.sin_port = htons(8080);          // порт 8080

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));

    // --- КРОК 3: почати слухати вхідні з'єднання ---
    listen(server_fd, 5);
    std::cout << "Server listening on port 8080..." << std::endl;

    // --- КРОК 4: чекати ОДНОГО клієнта (без поллу, без циклу - максимально просто) ---
    // Ця функція "зупиняє" програму і чекає, поки хтось не підключиться.
    int client_fd = accept(server_fd, nullptr, nullptr);
    std::cout << "Client connected!" << std::endl;

    // --- КРОК 5: прочитати те, що клієнт надіслав (сам HTTP-запит) ---
    char buffer[4096] = {0};
    read(client_fd, buffer, sizeof(buffer) - 1);
    std::cout << "===== Client sent this request =====\n" << buffer << std::endl;

    // --- КРОК 6: прочитати наш HTML-файл з диска ---
    std::ifstream file("index.html");
    std::stringstream fileContent;
    fileContent << file.rdbuf();
    std::string body = fileContent.str();

    // --- КРОК 7: побудувати HTTP-відповідь ---
    // ОБОВ'ЯЗКОВИЙ формат: status line, заголовки, ПОРОЖНІЙ рядок, потім тіло
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "\r\n" +
        body;

    // --- КРОК 8: відправити відповідь клієнту ---
    write(client_fd, response.c_str(), response.size());
    std::cout << "Response sent!" << std::endl;

    // --- КРОК 9: закрити з'єднання ---
    close(client_fd);
    close(server_fd);

    return 0;
}