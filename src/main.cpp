// ============================================================
//  esp32s3-cam-viewer — приём и воспроизведение MJPEG-потока
//  с платы GOOUUU ESP32-S3-CAM V1.5
//
//  Зависимости: OpenCV (libopencv-dev), pthread
//  Сборка:      make && make run URL=http://<IP>/stream
//  В CLion:     Open Project -> выбрать CMakeLists.txt
// ============================================================

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <cstring>
#include <csignal>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

// ----- Глобальные флаги -----
static std::atomic<bool> g_running{true};
static std::atomic<bool> g_reconnect{false};

// ----- Обработчик сигналов -----
static void signalHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        g_running = false;
    }
}

// ----- Параметры по умолчанию -----
struct StreamConfig {
    std::string url       = "http://192.168.4.1/stream";
    std::string snapshotUrl = "http://192.168.4.1/capture";
    int width             = 800;
    int height            = 600;
    bool fullscreen       = false;
    int reconnectDelayMs  = 2000;
    int maxReconnectTries = 0;  // 0 = бесконечно
};

// ----- Парсинг аргументов командной строки -----
static StreamConfig parseArgs(int argc, char* argv[]) {
    StreamConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--url" || arg == "-u") {
            if (i + 1 < argc) cfg.url = argv[++i];
        } else if (arg == "--snapshot" || arg == "-s") {
            if (i + 1 < argc) cfg.snapshotUrl = argv[++i];
        } else if (arg == "--width" || arg == "-w") {
            if (i + 1 < argc) cfg.width = std::atoi(argv[++i]);
        } else if (arg == "--height" || arg == "-h") {
            if (i + 1 < argc) cfg.height = std::atoi(argv[++i]);
        } else if (arg == "--fullscreen" || arg == "-f") {
            cfg.fullscreen = true;
        } else if (arg == "--help") {
            std::cout <<
                "Использование: esp32s3-cam-viewer [опции]\n"
                "  --url <URL>       URL MJPEG-потока (по умолчанию: http://192.168.4.1/stream)\n"
                "  --snapshot <URL>  URL для снимка     (по умолчанию: http://192.168.4.1/capture)\n"
                "  --width <N>       Ширина окна       (по умолчанию: 800)\n"
                "  --height <N>      Высота окна       (по умолчанию: 600)\n"
                "  --fullscreen      Полноэкранный режим\n"
                "  --help             Эта справка\n"
                "\n"
                "Пример:\n"
                "  esp32s3-cam-viewer --url http://192.168.4.1/stream\n";
            exit(0);
        } else if (arg.rfind("http", 0) == 0) {
            // Если передан просто URL без --url
            cfg.url = arg;
        }
    }
    return cfg;
}

// ----- Статистика FPS -----
class FPSCounter {
public:
    void tick() {
        frameCount_++;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastTime_).count();
        if (elapsed >= 1000) {
            fps_ = frameCount_ * 1000.0 / elapsed;
            frameCount_ = 0;
            lastTime_ = now;
        }
    }
    double get() const { return fps_; }
private:
    int frameCount_ = 0;
    double fps_ = 0.0;
    std::chrono::steady_clock::time_point lastTime_ = std::chrono::steady_clock::now();
};

// ----- Подключение к потоку с переподключением -----
static cv::VideoCapture openStream(const std::string& url) {
    cv::VideoCapture cap;
    // OpenCV умеет открывать MJPEG-потоки напрямую по HTTP
    cap.open(url, cv::CAP_FFMPEG);
    if (!cap.isOpened()) {
        // Пробуем без явного backend
        cap.open(url);
    }
    return cap;
}

// ----- Сохранение снимка -----
static bool saveSnapshot(const std::string& snapshotUrl, const std::string& filename) {
    cv::VideoCapture cap;
    cap.open(snapshotUrl);
    if (!cap.isOpened()) return false;
    cv::Mat frame;
    cap >> frame;
    if (frame.empty()) return false;
    return cv::imwrite(filename, frame);
}

// ----- Главный цикл -----
int main(int argc, char* argv[]) {
    // Регистрация сигналов
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    StreamConfig cfg = parseArgs(argc, argv);

    std::cout << "========================================\n";
    std::cout << "  ESP32-S3-CAM Viewer (GOOUUU V1.5)\n";
    std::cout << "========================================\n";
    std::cout << "URL потока:  " << cfg.url << "\n";
    std::cout << "URL снимка:  " << cfg.snapshotUrl << "\n";
    std::cout << "Окно:        " << cfg.width << "x" << cfg.height << "\n";
    std::cout << "========================================\n";
    std::cout << "Управление:\n";
    std::cout << "  ESC / Q  — выход\n";
    std::cout << "  S        — сохранить снимок в файл\n";
    std::cout << "  R        — переподключиться\n";
    std::cout << "  F        — полный экран вкл/выкл\n";
    std::cout << "========================================\n\n";

    // Окно
    cv::namedWindow("ESP32-S3-CAM", cv::WINDOW_NORMAL);
    cv::resizeWindow("ESP32-S3-CAM", cfg.width, cfg.height);
    if (cfg.fullscreen) {
        cv::setWindowProperty("ESP32-S3-CAM", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
    }

    // Подключение
    std::cout << "[+] Подключение к потоку..." << std::endl;
    cv::VideoCapture cap = openStream(cfg.url);

    if (!cap.isOpened()) {
        std::cerr << "[!] Не удалось подключиться к " << cfg.url << std::endl;
        std::cerr << "    Проверьте: питание платы, WiFi-соединение, IP-адрес." << std::endl;
        std::cerr << "    Переподключение каждые " << cfg.reconnectDelayMs << " мс..." << std::endl;
    } else {
        std::cout << "[+] Поток открыт успешно." << std::endl;
    }

    FPSCounter fpsCounter;
    int reconnectTries = 0;
    int snapshotIndex = 0;

    while (g_running) {
        // Переподключение
        if (g_reconnect || !cap.isOpened()) {
            g_reconnect = false;
            std::cout << "[~] Переподключение..." << std::endl;
            cap.release();
            std::this_thread::sleep_for(
                std::chrono::milliseconds(cfg.reconnectDelayMs));
            cap = openStream(cfg.url);
            if (cap.isOpened()) {
                std::cout << "[+] Переподключено." << std::endl;
                reconnectTries = 0;
            } else {
                reconnectTries++;
                if (cfg.maxReconnectTries > 0 &&
                    reconnectTries >= cfg.maxReconnectTries) {
                    std::cerr << "[!] Превышен лимт попыток. Выход." << std::endl;
                    break;
                }
                std::cerr << "[!] Попытка " << reconnectTries << " неудачна."
                          << std::endl;
                continue;
            }
        }

        // Чтение кадра
        cv::Mat frame;
        if (!cap.read(frame) || frame.empty()) {
            std::cerr << "[!] Пустой кадр. Переподключение..." << std::endl;
            cap.release();
            g_reconnect = true;
            continue;
        }

        fpsCounter.tick();

        // Наложение информации на кадр
        std::string info = cv::format("FPS: %.1f  |  %dx%d  |  ESC-выход  S-снимок  R-реконнект",
            fpsCounter.get(), frame.cols, frame.rows);
        cv::putText(frame, info, cv::Point(10, 25),
            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1, cv::LINE_AA);

        cv::imshow("ESP32-S3-CAM", frame);

        // Обработка клавиш
        int key = cv::waitKey(1) & 0xFF;
        if (key == 27 || key == 'q' || key == 'Q') {
            std::cout << "[*] Выход по запросу пользователя." << std::endl;
            break;
        } else if (key == 's' || key == 'S') {
            std::string filename = cv::format("snapshot_%04d.jpg", snapshotIndex++);
            // Сначала пробуем через /capture, потом из текущего кадра
            bool ok = saveSnapshot(cfg.snapshotUrl, filename);
            if (!ok) {
                ok = cv::imwrite(filename, frame);
            }
            if (ok) {
                std::cout << "[+] Снимок сохранён: " << filename << std::endl;
            } else {
                std::cerr << "[!] Не удалось сохранить снимок." << std::endl;
            }
        } else if (key == 'r' || key == 'R') {
            g_reconnect = true;
        } else if (key == 'f' || key == 'F') {
            static bool fs = cfg.fullscreen;
            fs = !fs;
            cv::setWindowProperty("ESP32-S3-CAM", cv::WND_PROP_FULLSCREEN,
                fs ? cv::WINDOW_FULLSCREEN : cv::WINDOW_NORMAL);
        }
    }

    // Очистка
    cap.release();
    cv::destroyAllWindows();
    std::cout << "[*] Завершено." << std::endl;
    return 0;
}
