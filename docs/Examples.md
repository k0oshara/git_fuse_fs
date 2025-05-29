# Примеры использования GitFS

Этот документ содержит практические примеры использования GitFS в различных сценариях.

## Базовые примеры

### Простое монтирование и просмотр

```bash
# Сборка GitFS
make

# Создание точки монтирования
mkdir ~/git-mount

# Монтирование текущего репозитория
./gitfs . ~/git-mount -f

# В другом терминале - просмотр файлов
ls ~/git-mount/
cat ~/git-mount/README.md
find ~/git-mount -name "*.c"

# Размонтирование
fusermount3 -u ~/git-mount
```

### Монтирование в фоновом режиме

```bash
# Монтирование в демон режиме
./gitfs /path/to/repo ~/git-mount -o ro,allow_other

# Проверка монтирования
mount | grep gitfs
df -h ~/git-mount

# Работа с файлами
ls ~/git-mount/
cat ~/git-mount/src/main.c

# Размонтирование
fusermount3 -u ~/git-mount
```

## Анализ кода

### Поиск файлов по типу

```bash
# Монтирование репозитория
./gitfs /path/to/project ~/project-mount -f &

# Поиск всех C файлов
find ~/project-mount -name "*.c" -type f

# Поиск всех заголовочных файлов
find ~/project-mount -name "*.h" -type f

# Подсчет строк кода
find ~/project-mount -name "*.c" -exec wc -l {} + | tail -1

# Поиск TODO комментариев
grep -r "TODO" ~/project-mount/

# Размонтирование
fusermount3 -u ~/project-mount
```

### Анализ структуры проекта

```bash
# Монтирование
./gitfs /path/to/large-project ~/analysis-mount -f &

# Визуализация структуры директорий
tree ~/analysis-mount/

# Анализ размеров файлов
find ~/analysis-mount -type f -exec ls -lh {} + | sort -k5 -hr | head -20

# Поиск самых больших файлов
find ~/analysis-mount -type f -exec du -h {} + | sort -hr | head -10

# Подсчет файлов по типам
find ~/analysis-mount -type f | sed 's/.*\.//' | sort | uniq -c | sort -nr

# Размонтирование
fusermount3 -u ~/analysis-mount
```

### Сравнение версий

```bash
# Монтирование старой версии проекта
./gitfs /path/to/old-version ~/old-mount -f &

# Монтирование новой версии
./gitfs /path/to/new-version ~/new-mount -f &

# Сравнение файлов
diff ~/old-mount/src/main.c ~/new-mount/src/main.c

# Сравнение директорий
diff -r ~/old-mount/src/ ~/new-mount/src/

# Поиск новых файлов
comm -13 <(find ~/old-mount -type f | sort) <(find ~/new-mount -type f | sort)

# Поиск удаленных файлов
comm -23 <(find ~/old-mount -type f | sort) <(find ~/new-mount -type f | sort)

# Размонтирование
fusermount3 -u ~/old-mount
fusermount3 -u ~/new-mount
```

## Интеграция с инструментами

### Использование с IDE

```bash
# Монтирование проекта для просмотра в IDE
./gitfs /path/to/readonly-project ~/ide-mount -o allow_other &

# Открытие в VS Code (только для чтения)
code ~/ide-mount/

# Открытие в Vim
vim ~/ide-mount/src/main.c

# Размонтирование после работы
fusermount3 -u ~/ide-mount
```

### Интеграция с grep и ack

```bash
# Монтирование
./gitfs /path/to/project ~/search-mount -f &

# Поиск с grep
grep -r "function_name" ~/search-mount/

# Поиск с ack (если установлен)
ack "pattern" ~/search-mount/

# Поиск с ripgrep (если установлен)
rg "pattern" ~/search-mount/

# Размонтирование
fusermount3 -u ~/search-mount
```

### Использование с ctags

```bash
# Монтирование
./gitfs /path/to/project ~/ctags-mount -f &

# Генерация tags файла
cd ~/ctags-mount
ctags -R .

# Использование tags в vim
vim -t function_name

# Размонтирование
fusermount3 -u ~/ctags-mount
```

## Автоматизация

### Скрипт для анализа проекта

```bash
#!/bin/bash
# analyze_project.sh

PROJECT_PATH="$1"
MOUNT_POINT="/tmp/gitfs-analysis-$$"

if [ -z "$PROJECT_PATH" ]; then
    echo "Использование: $0 <path-to-git-repo>"
    exit 1
fi

# Функция очистки
cleanup() {
    fusermount3 -u "$MOUNT_POINT" 2>/dev/null || true
    rmdir "$MOUNT_POINT" 2>/dev/null || true
}

trap cleanup EXIT

# Создание точки монтирования
mkdir -p "$MOUNT_POINT"

# Монтирование
echo "Анализ проекта: $PROJECT_PATH"
./gitfs "$PROJECT_PATH" "$MOUNT_POINT" -f &
GITFS_PID=$!

# Ожидание монтирования
sleep 2

# Анализ
echo "=== Структура проекта ==="
find "$MOUNT_POINT" -type d | head -20

echo -e "\n=== Типы файлов ==="
find "$MOUNT_POINT" -type f | sed 's/.*\.//' | sort | uniq -c | sort -nr | head -10

echo -e "\n=== Самые большие файлы ==="
find "$MOUNT_POINT" -type f -exec du -h {} + | sort -hr | head -10

echo -e "\n=== Количество строк кода ==="
find "$MOUNT_POINT" -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" | \
    xargs wc -l | tail -1

echo -e "\n=== TODO комментарии ==="
grep -r "TODO\|FIXME\|XXX" "$MOUNT_POINT" 2>/dev/null | wc -l

# Завершение
kill $GITFS_PID 2>/dev/null || true
```

### Скрипт для мониторинга изменений

```bash
#!/bin/bash
# monitor_changes.sh

OLD_REPO="$1"
NEW_REPO="$2"
OLD_MOUNT="/tmp/gitfs-old-$$"
NEW_MOUNT="/tmp/gitfs-new-$$"

if [ -z "$OLD_REPO" ] || [ -z "$NEW_REPO" ]; then
    echo "Использование: $0 <old-repo> <new-repo>"
    exit 1
fi

# Функция очистки
cleanup() {
    fusermount3 -u "$OLD_MOUNT" 2>/dev/null || true
    fusermount3 -u "$NEW_MOUNT" 2>/dev/null || true
    rmdir "$OLD_MOUNT" "$NEW_MOUNT" 2>/dev/null || true
}

trap cleanup EXIT

# Создание точек монтирования
mkdir -p "$OLD_MOUNT" "$NEW_MOUNT"

# Монтирование
./gitfs "$OLD_REPO" "$OLD_MOUNT" -f &
OLD_PID=$!

./gitfs "$NEW_REPO" "$NEW_MOUNT" -f &
NEW_PID=$!

sleep 3

echo "=== Сравнение репозиториев ==="
echo "Старый: $OLD_REPO"
echo "Новый: $NEW_REPO"

echo -e "\n=== Новые файлы ==="
comm -13 <(find "$OLD_MOUNT" -type f | sort) <(find "$NEW_MOUNT" -type f | sort)

echo -e "\n=== Удаленные файлы ==="
comm -23 <(find "$OLD_MOUNT" -type f | sort) <(find "$NEW_MOUNT" -type f | sort)

echo -e "\n=== Измененные файлы ==="
for file in $(comm -12 <(find "$OLD_MOUNT" -type f | sort) <(find "$NEW_MOUNT" -type f | sort)); do
    old_file="$OLD_MOUNT${file#$NEW_MOUNT}"
    if ! cmp -s "$old_file" "$file" 2>/dev/null; then
        echo "$file"
    fi
done

# Завершение
kill $OLD_PID $NEW_PID 2>/dev/null || true
```

### Интеграция с systemd

Создайте файл `/etc/systemd/system/gitfs@.service`:

```ini
[Unit]
Description=GitFS mount for %i
After=local-fs.target
Requires=local-fs.target

[Service]
Type=forking
User=gitfs
Group=gitfs
ExecStartPre=/bin/mkdir -p /mnt/gitfs/%i
ExecStart=/usr/local/bin/gitfs /var/git/%i /mnt/gitfs/%i -o allow_other
ExecStop=/bin/fusermount3 -u /mnt/gitfs/%i
ExecStopPost=/bin/rmdir /mnt/gitfs/%i
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Использование:

```bash
# Создание пользователя для GitFS
sudo useradd -r -s /bin/false gitfs

# Создание директорий
sudo mkdir -p /var/git /mnt/gitfs
sudo chown gitfs:gitfs /var/git /mnt/gitfs

# Копирование репозитория
sudo cp -r /path/to/repo /var/git/myproject
sudo chown -R gitfs:gitfs /var/git/myproject

# Запуск сервиса
sudo systemctl start gitfs@myproject

# Автозапуск
sudo systemctl enable gitfs@myproject

# Проверка статуса
sudo systemctl status gitfs@myproject

# Просмотр файлов
ls /mnt/gitfs/myproject/
```

## Использование с Docker

### Dockerfile для GitFS

```dockerfile
FROM ubuntu:20.04

# Установка зависимостей
RUN apt-get update && apt-get install -y \
    libfuse3-dev \
    libgit2-dev \
    pkg-config \
    build-essential \
    git \
    && rm -rf /var/lib/apt/lists/*

# Копирование исходного кода
COPY . /app
WORKDIR /app

# Сборка
RUN make

# Создание пользователя
RUN useradd -r -s /bin/false gitfs

# Создание директорий
RUN mkdir -p /repo /mount
RUN chown gitfs:gitfs /repo /mount

USER gitfs

# Точка входа
ENTRYPOINT ["./gitfs"]
CMD ["/repo", "/mount", "-f"]
```

### Docker Compose

```yaml
version: '3.8'

services:
  gitfs:
    build: .
    privileged: true  # Требуется для FUSE
    volumes:
      - ./my-repo:/repo:ro
      - gitfs-mount:/mount
    devices:
      - /dev/fuse:/dev/fuse
    cap_add:
      - SYS_ADMIN

volumes:
  gitfs-mount:
```

Использование:

```bash
# Сборка и запуск
docker-compose up -d

# Просмотр файлов
docker exec -it gitfs_gitfs_1 ls /mount

# Остановка
docker-compose down
```

## Производительность и оптимизация

### Тестирование производительности

```bash
#!/bin/bash
# performance_test.sh

REPO_PATH="$1"
MOUNT_POINT="/tmp/gitfs-perf-$$"

if [ -z "$REPO_PATH" ]; then
    echo "Использование: $0 <repo-path>"
    exit 1
fi

cleanup() {
    fusermount3 -u "$MOUNT_POINT" 2>/dev/null || true
    rmdir "$MOUNT_POINT" 2>/dev/null || true
}

trap cleanup EXIT

mkdir -p "$MOUNT_POINT"

echo "=== Тест производительности GitFS ==="
echo "Репозиторий: $REPO_PATH"

# Тест времени монтирования
echo -n "Время монтирования: "
start_time=$(date +%s.%N)
./gitfs "$REPO_PATH" "$MOUNT_POINT" -f &
GITFS_PID=$!

# Ожидание монтирования
while [ ! -d "$MOUNT_POINT" ] || [ -z "$(ls -A "$MOUNT_POINT" 2>/dev/null)" ]; do
    sleep 0.1
done

end_time=$(date +%s.%N)
mount_time=$(echo "$end_time - $start_time" | bc -l)
echo "${mount_time}s"

# Тест листинга директории
echo -n "Время листинга корневой директории: "
start_time=$(date +%s.%N)
ls "$MOUNT_POINT" >/dev/null
end_time=$(date +%s.%N)
list_time=$(echo "$end_time - $start_time" | bc -l)
echo "${list_time}s"

# Тест чтения файлов
echo -n "Время чтения 10 файлов: "
start_time=$(date +%s.%N)
find "$MOUNT_POINT" -type f | head -10 | while read file; do
    cat "$file" >/dev/null 2>&1
done
end_time=$(date +%s.%N)
read_time=$(echo "$end_time - $start_time" | bc -l)
echo "${read_time}s"

# Статистика
file_count=$(find "$MOUNT_POINT" -type f | wc -l)
dir_count=$(find "$MOUNT_POINT" -type d | wc -l)
total_size=$(du -sh "$MOUNT_POINT" | cut -f1)

echo "Файлов: $file_count"
echo "Директорий: $dir_count"
echo "Общий размер: $total_size"

kill $GITFS_PID 2>/dev/null || true
```

### Мониторинг ресурсов

```bash
#!/bin/bash
# monitor_gitfs.sh

GITFS_PID=$(pgrep gitfs)

if [ -z "$GITFS_PID" ]; then
    echo "GitFS не запущен"
    exit 1
fi

echo "Мониторинг GitFS (PID: $GITFS_PID)"
echo "Нажмите Ctrl+C для остановки"

while true; do
    # Использование CPU и памяти
    ps_output=$(ps -p $GITFS_PID -o pid,pcpu,pmem,vsz,rss,comm --no-headers 2>/dev/null)
    
    if [ -z "$ps_output" ]; then
        echo "GitFS процесс завершился"
        break
    fi
    
    echo "$(date): $ps_output"
    
    # Статистика файловой системы
    mount_point=$(mount | grep gitfs | awk '{print $3}' | head -1)
    if [ -n "$mount_point" ]; then
        echo "  Точка монтирования: $mount_point"
        echo "  Открытые файлы: $(lsof +D "$mount_point" 2>/dev/null | wc -l)"
    fi
    
    sleep 5
done
```

## Отладка и диагностика

### Подробное логирование

```bash
# Запуск с максимальным логированием
./gitfs /path/to/repo /mount/point -f -d 2>&1 | tee gitfs-debug.log

# Фильтрация логов по типу операций
grep "getattr" gitfs-debug.log
grep "readdir" gitfs-debug.log
grep "read" gitfs-debug.log

# Анализ производительности операций
grep -E "resolve_oid|load_blob|load_tree" gitfs-debug.log
```

### Трассировка системных вызовов

```bash
# Трассировка с strace
strace -f -e trace=file ./gitfs /path/to/repo /mount/point -f

# Трассировка только операций чтения
strace -f -e trace=read,readdir ./gitfs /path/to/repo /mount/point -f

# Сохранение трассировки в файл
strace -f -o gitfs-trace.log ./gitfs /path/to/repo /mount/point -f
```

### Профилирование с perf

```bash
# Запись профиля
perf record -g ./gitfs /path/to/repo /mount/point -f

# Анализ профиля
perf report

# Профилирование конкретных функций
perf record -g -e cycles:u ./gitfs /path/to/repo /mount/point -f
```

## Полезные алиасы и функции

Добавьте в ваш `.bashrc` или `.zshrc`:

```bash
# Алиас для быстрого монтирования
alias gitfs-mount='./gitfs'

# Функция для безопасного монтирования
gitfs_safe_mount() {
    local repo="$1"
    local mount="$2"
    
    if [ -z "$repo" ] || [ -z "$mount" ]; then
        echo "Использование: gitfs_safe_mount <repo> <mount-point>"
        return 1
    fi
    
    mkdir -p "$mount"
    ./gitfs "$repo" "$mount" -f &
    local pid=$!
    
    # Ожидание монтирования
    for i in {1..30}; do
        if mountpoint -q "$mount" 2>/dev/null; then
            echo "GitFS смонтирован в $mount (PID: $pid)"
            return 0
        fi
        sleep 1
    done
    
    echo "Ошибка монтирования"
    kill $pid 2>/dev/null
    return 1
}

# Функция для безопасного размонтирования
gitfs_safe_umount() {
    local mount="$1"
    
    if [ -z "$mount" ]; then
        echo "Использование: gitfs_safe_umount <mount-point>"
        return 1
    fi
    
    if mountpoint -q "$mount" 2>/dev/null; then
        fusermount3 -u "$mount" || fusermount -u "$mount" || umount "$mount"
        echo "Размонтировано: $mount"
    else
        echo "Не смонтировано: $mount"
    fi
}

# Функция для анализа Git репозитория
gitfs_analyze() {
    local repo="$1"
    local mount="/tmp/gitfs-analyze-$$"
    
    if [ -z "$repo" ]; then
        echo "Использование: gitfs_analyze <repo-path>"
        return 1
    fi
    
    gitfs_safe_mount "$repo" "$mount"
    
    echo "=== Анализ репозитория: $repo ==="
    echo "Файлов: $(find "$mount" -type f | wc -l)"
    echo "Директорий: $(find "$mount" -type d | wc -l)"
    echo "Размер: $(du -sh "$mount" | cut -f1)"
    
    echo -e "\nТипы файлов:"
    find "$mount" -type f | sed 's/.*\.//' | sort | uniq -c | sort -nr | head -10
    
    gitfs_safe_umount "$mount"
    rmdir "$mount" 2>/dev/null
}
```

Эти примеры показывают различные способы использования GitFS для анализа кода, автоматизации задач и интеграции с существующими инструментами разработки.