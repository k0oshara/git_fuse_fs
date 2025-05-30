# Быстрый старт

Это руководство поможет вам быстро начать работу с GitFS.

## Предварительные требования

### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install libfuse3-dev libgit2-dev pkg-config build-essential git
```

### Linux (CentOS/RHEL/Fedora)
```bash
# CentOS/RHEL
sudo yum install fuse3-devel libgit2-devel pkgconfig gcc make git

# Fedora
sudo dnf install fuse3-devel libgit2-devel pkgconfig gcc make git
```

### macOS
```bash
# Установка Homebrew (если не установлен)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Установка зависимостей
brew install libgit2 macfuse pkg-config
```

## Установка

### Из исходного кода

```bash
# 1. Клонирование репозитория
git clone https://github.com/username/git_fuse_fs.git
cd git_fuse_fs

# 2. Сборка
make

# 3. Проверка сборки
./gitfs --help
```

### Проверка установки

```bash
# Проверка зависимостей
pkg-config --exists fuse3 libgit2 && echo "Зависимости найдены" || echo "Зависимости отсутствуют"

# Проверка FUSE
ls /dev/fuse && echo "FUSE доступен" || echo "FUSE недоступен"

# Проверка прав доступа к FUSE
groups | grep -q fuse && echo "Пользователь в группе fuse" || echo "Добавьте пользователя в группу fuse"
```

## Первый запуск

### Шаг 1: Подготовка тестового репозитория

```bash
# Создание тестового репозитория
mkdir test-repo
cd test-repo
git init
git config user.email "test@example.com"
git config user.name "Test User"

# Создание тестовых файлов
echo "Hello, GitFS!" > hello.txt
mkdir docs
echo "# Documentation" > docs/README.md
echo "Some content" > docs/guide.txt

# Коммит
git add .
git commit -m "Initial commit"
cd ..
```

### Шаг 2: Монтирование

```bash
# Создание точки монтирования
mkdir mount-point

# Монтирование в foreground режиме (для отладки)
./gitfs test-repo mount-point -f
```

### Шаг 3: Тестирование (в новом терминале)

```bash
# Просмотр содержимого
ls mount-point/
# Вывод: docs  hello.txt

# Чтение файла
cat mount-point/hello.txt
# Вывод: Hello, GitFS!

# Просмотр директории
ls mount-point/docs/
# Вывод: README.md  guide.txt

# Чтение файла из поддиректории
cat mount-point/docs/README.md
# Вывод: # Documentation
```

### Шаг 4: Размонтирование

```bash
# В терминале с GitFS нажмите Ctrl+C или в другом терминале:
fusermount3 -u mount-point

# Или на macOS:
umount mount-point
```

## Режимы запуска

### Демон режим (фоновый)

```bash
# Запуск в фоне
./gitfs test-repo mount-point -o ro,allow_other

# Проверка монтирования
mount | grep gitfs

# Размонтирование
fusermount3 -u mount-point
```

### Отладочный режим

```bash
# Запуск с подробными логами
./gitfs test-repo mount-point -f -d

# Сохранение логов в файл
./gitfs test-repo mount-point -f 2> gitfs.log
```

## Полезные опции

### Основные опции FUSE

```bash
# Разрешить доступ другим пользователям
./gitfs repo mount-point -o allow_other

# Автоматическое размонтирование при завершении
./gitfs repo mount-point -o auto_unmount

# Только для чтения (по умолчанию)
./gitfs repo mount-point -o ro

# Комбинирование опций
./gitfs repo mount-point -f -o ro,allow_other,auto_unmount
```

### Опции производительности

```bash
# Включение кэширования ядра (по умолчанию включено)
./gitfs repo mount-point -o kernel_cache

# Отключение кэширования (для отладки)
./gitfs repo mount-point -o no_kernel_cache
```

## Проверка работы

### Автоматические тесты

```bash
# Запуск всех тестов
make test

# Ручной запуск тестовой утилиты
cd test
./test_gitfs
```

### Ручное тестирование

```bash
# Создание тестового скрипта
cat > test_manual.sh << 'EOF'
#!/bin/bash
set -e

echo "=== Тест GitFS ==="

# Подготовка
mkdir -p test-repo mount-point
cd test-repo
git init
git config user.email "test@example.com"
git config user.name "Test User"
echo "Test content" > test.txt
git add test.txt
git commit -m "Test commit"
cd ..

# Монтирование
./gitfs test-repo mount-point -f &
GITFS_PID=$!
sleep 2

# Тестирование
echo "Содержимое mount-point:"
ls -la mount-point/

echo "Содержимое test.txt:"
cat mount-point/test.txt

echo "Проверка прав доступа:"
ls -l mount-point/test.txt

# Очистка
kill $GITFS_PID
sleep 1
fusermount3 -u mount-point || true
rm -rf test-repo mount-point

echo "=== Тест завершен успешно ==="
EOF

chmod +x test_manual.sh
./test_manual.sh
```

## Следующие шаги

После успешного запуска рекомендуется:

1. Изучить [Руководство пользователя](User-Guide.md) для подробного описания возможностей
2. Ознакомиться с [Примерами использования](Examples.md)
3. При возникновении проблем обратиться к [Устранению неполадок](Troubleshooting.md)
4. Для разработки изучить [Руководство разработчика](Developer-Guide.md)

## Получение помощи

Если что-то не работает:

1. Проверьте [FAQ](FAQ.md)
2. Изучите [Устранение неполадок](Troubleshooting.md)
3. Создайте [Issue на GitHub](https://github.com/username/git_fuse_fs/issues)
4. Обратитесь к [сообществу](https://github.com/username/git_fuse_fs/discussions)