# ESP32-S3-CAM Viewer (GOOUUU V1.5)

Приём и воспроизведение MJPEG-видеопотока с платы **GOOUUU ESP32-S3-CAM V1.5** на Linux.

## Требования

### Системные пакеты (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
    libopencv-dev libcurl4-openssl-dev
```

### Arch Linux

```bash
sudo pacman -S base-devel cmake opencv curl
```

### Fedora

```bash
sudo dnf install gcc-c++ cmake opencv-devel libcurl-devel
```

## Сборка

### Вариант 1: через Make

```bash
cd esp32s3-cam-viewer
make          # сборка
make run      # сборка + запуск (URL по умолчанию)
```

Запуск с своим URL:

```bash
make run URL=http://192.168.4.1/stream
```

### Вариант 2: через CMake напрямую

```bash
cd esp32s3-cam-viewer
cmake -B build -DCMAKE_BUILD_TYPE=Release .
cmake --build build -j$(nproc)
./build/esp32s3-cam-viewer --url http://192.168.4.1/stream
```

### Вариант 3: в CLion

1. File → Open → выбрать `CMakeLists.txt` в папке проекта.
2. CLion автоматически определит конфигурацию CMake.
3. Убедитесь, что в Toolchain выбран GCC (`/usr/bin/gcc` и `/usr/bin/g++`).
4. Нажмите Run (зелёная стрелка).
5. Аргументы командной строки можно задать в
   Run → Edit Configurations → Program arguments:
   ```
    --url http://192.168.4.1/stream
   ```

## Использование

```bash
# Базовый запуск (ESP32S3-CAM access point)
./build/esp32s3-cam-viewer

# Свой URL
./build/esp32s3-cam-viewer --url http://192.168.4.1/stream

# Полный экран
./build/esp32s3-cam-viewer --url http://192.168.4.1/stream --fullscreen

# Свой размер окна
./build/esp32s3-cam-viewer --url http://192.168.4.1/stream --width 1024 --height 768
```

### Управление

| Клавиша | Действие |
|---------|----------|
| ESC / Q | Выход |
| S       | Сохранить снимок в файл `snapshot_XXXX.jpg` |
| R       | Переподключиться к потоку |
| F       | Полноэкранный режим вкл/выкл |

## Аргументы

```
--url <URL>        URL MJPEG-потока (по умолчанию: http://192.168.4.1/stream)
--snapshot <URL>   URL для снимка (по умолчанию: http://192.168.4.1/capture)
--width <N>        Ширина окна (по умолчанию: 800)
--height <N>       Высота окна (по умолчанию: 600)
--fullscreen       Полноэкранный режим
--help             Справка
```

## Структура проекта

```
esp32s3-cam-viewer/
├── CMakeLists.txt        — конфигурация CMake (для CLion)
├── Makefile              — обёртка над CMake (make + gcc)
├── README.md             — этот файл
├── src/
│   └── main.cpp          — основная программа (OpenCV)
└── firmware/
    └── esp32s3_cam.ino   — прошивка для ESP32-S3-CAM (Arduino)
```

## Подключение камеры

Прошивка находится в `firmware/esp32s3_cam.ino`.

### VS Code + PlatformIO

В проект добавлен `platformio.ini`: PlatformIO собирает код из `firmware/`,
не затрагивая Linux-просмотрщик в `src/`.

1. Установите расширение **PlatformIO IDE** в VS Code и откройте корневую
    папку проекта.
2. В **PlatformIO Project Tasks → esp32-s3-cam → General** выполните **Build**,
    затем **Upload**.
3. Откройте **Monitor**. Скорость Serial Monitor — `115200`.

В конфигурации указан порт `/dev/ttyUSB0` (CH340); если система назначит
другой порт, измените `upload_port` и `monitor_port` в `platformio.ini`.

### Arduino IDE

Выберите плату `ESP32S3 Dev Module` и включите PSRAM (Octal), затем загрузите
прошивку.

Для доступа к последовательному порту в Linux добавьте пользователя в группу
`dialout` и заново войдите в систему:

```bash
sudo usermod -aG dialout "$USER"
```

По умолчанию плата создаёт Wi-Fi сеть `ESP32S3-CAM` с паролем `camviewer`.
Подключите компьютер к этой сети и запустите `make run` или приложение.

Чтобы использовать домашний роутер вместо точки доступа, укажите его SSID
и пароль в начале файла прошивки и загрузите её повторно. Если подключение
к роутеру не удастся за 15 секунд, плата автоматически создаст точку доступа.
Адрес камеры в режиме точки доступа — `192.168.4.1`; IP в режиме роутера
появится в Serial Monitor (115200 baud).

Потоки доступны по адресам:

- `http://<IP>/`        — веб-страница с потоком
- `http://<IP>/stream`  — MJPEG-поток
- `http://<IP>/capture` — одиночный снимок (JPEG)

`lsusb` показывает плату как USB-UART адаптер CH340; видео идёт по Wi-Fi,
а не как USB-видеоустройство. Для проверки загрузки прошивки подключите
Serial Monitor к соответствующему `/dev/ttyUSB*` на скорости 115200 baud.
