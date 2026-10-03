# ESP32-S3-CAM Viewer

Просмотр MJPEG-видео с ESP32-S3-CAM на macOS, Windows и Linux.

По умолчанию прошивка запускает точку доступа `ESP32S3-CAM` с паролем
`camviewer`. Подключите к ней компьютер; адрес камеры — `192.168.4.1`, и его
можно не указывать при запуске viewer. Для работы через домашний роутер задайте
его SSID и пароль в `firmware/esp32s3_cam.ino`; назначенный адрес появится в
Serial Monitor как `WiFi IP:`.

## Прошивка камеры одной командой

Подключите плату по USB. В Linux один раз добавьте пользователя в группу
`dialout` и заново войдите в систему, чтобы получить доступ к serial-порту:

```bash
sudo usermod -aG dialout "$USER"
```

Изменение не применяется к уже запущенным терминалам. Чтобы продолжить без
выхода из системы, выполните `newgrp dialout` и проверьте `id -nG`: в выводе
должна быть группа `dialout`. Запускайте следующую команду в этом же терминале.
Либо полностью выйдите из сеанса Linux и войдите снова.

Из корня проекта выполните команду: она соберёт и загрузит прошивку, после
чего откроет Serial Monitor.

```bash
make camera
```

## macOS

1. Установите Xcode Command Line Tools и Homebrew:

```bash
xcode-select --install
```

Инструкция по установке Homebrew: [brew.sh](https://brew.sh/).

2. Установите Git, CMake и OpenCV:

```bash
brew install git cmake opencv
```

## Windows

1. Установите [MSYS2](https://www.msys2.org/), затем откройте терминал **MSYS2 UCRT64**.
2. Обновите MSYS2. Если установщик попросит закрыть терминал, откройте его снова
   и повторите команду:

```bash
pacman -Syu
```

3. Установите компилятор, Git, CMake, Ninja и OpenCV:

```bash
pacman -S --needed git mingw-w64-ucrt-x86_64-toolchain \
   mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
   mingw-w64-ucrt-x86_64-opencv
```

Все дальнейшие команды Windows выполняйте в терминале **MSYS2 UCRT64**.

## Linux

Установите Git, CMake, компилятор C++ и OpenCV для своего дистрибутива.

Ubuntu / Debian:

```bash
sudo apt update
sudo apt install -y git cmake g++ libopencv-dev
```

Fedora:

```bash
sudo dnf install git cmake gcc-c++ opencv-devel
```

Arch Linux:

```bash
sudo pacman -S --needed git cmake gcc opencv
```

## Скачать, собрать и запустить

> Перед запуском `make run` обязательно:
> 1. подключитесь к Wi‑Fi сети камеры `ESP32S3-CAM` (пароль `camviewer`);
> 2. выполните сборку проекта;
> 3. только после этого запускайте `make run`.
> 4. Меню прошивки: http://192.168.4.1

Клонируйте репозиторий и перейдите в его папку:

```bash
git clone https://github.com/datikken/esp32-s3-camviewer.git
cd esp32-s3-camviewer
```

macOS:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
   -DCMAKE_PREFIX_PATH="$(brew --prefix opencv)"
cmake --build build --parallel
./build/esp32s3-cam-viewer --url http://192.168.4.1/stream
```

Linux:

```bash
make build
make run URL=http://192.168.4.1/stream
```

Windows (в терминале MSYS2 UCRT64):

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
   -DCMAKE_PREFIX_PATH="$MINGW_PREFIX"
cmake --build build --parallel
./build/esp32s3-cam-viewer.exe --url http://192.168.4.1/stream
```

Если камера подключена к домашнему роутеру, замените `192.168.4.1` на адрес из
Serial Monitor. В режиме точки доступа подключите компьютер к `ESP32S3-CAM`.


## TODO 
RTP/UDP - https://habr.com/ru/articles/987604/