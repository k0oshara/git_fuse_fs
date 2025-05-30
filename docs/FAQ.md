# Часто задаваемые вопросы (FAQ)

## Общие вопросы

### Что такое GitFS?

GitFS - это файловая система FUSE, которая позволяет монтировать Git репозитории как обычные директории в файловой системе. Это дает возможность просматривать содержимое Git репозитория через файловый менеджер или стандартные утилиты командной строки.

### Зачем нужен GitFS?

GitFS полезен в следующих случаях:

- **Анализ кода**: Просмотр структуры проекта без клонирования
- **Интеграция**: Использование Git репозиториев в существующих рабочих процессах
- **Исследование**: Изучение больших репозиториев без загрузки всех файлов
- **Автоматизация**: Интеграция с инструментами, которые работают с файловой системой

### Какие операционные системы поддерживаются?

- **Linux**: Полная поддержка (Ubuntu, Debian, CentOS, Fedora, Arch)
- **macOS**: Поддерживается с macFUSE
- **Windows**: Не поддерживается (FUSE недоступен)

## Установка и сборка

### Ошибка "fuse3 not found" при сборке

```bash
# Ubuntu/Debian
sudo apt-get install libfuse3-dev

# CentOS/RHEL
sudo yum install fuse3-devel

# Fedora
sudo dnf install fuse3-devel

# macOS
brew install macfuse
```

### Ошибка "libgit2 not found"

```bash
# Ubuntu/Debian
sudo apt-get install libgit2-dev

# CentOS/RHEL
sudo yum install libgit2-devel

# Fedora
sudo dnf install libgit2-devel

# macOS
brew install libgit2
```

### Ошибка "pkg-config not found"

```bash
# Ubuntu/Debian
sudo apt-get install pkg-config

# CentOS/RHEL/Fedora
sudo yum install pkgconfig  # или sudo dnf install pkgconfig

# macOS
brew install pkg-config
```

### Не удается скачать uthash.h

```bash
# Ручная загрузка
mkdir -p include
wget -O include/uthash.h https://raw.githubusercontent.com/troydhanson/uthash/master/src/uthash.h

# Или с помощью curl
curl -o include/uthash.h https://raw.githubusercontent.com/troydhanson/uthash/master/src/uthash.h
```

## Использование

### Как монтировать репозиторий?

```bash
# Базовое монтирование
./gitfs /path/to/repo /path/to/mount

# С дополнительными опциями
./gitfs /path/to/repo /path/to/mount -o ro,allow_other

# В режиме отладки
./gitfs /path/to/repo /path/to/mount -f
```

### Как размонтировать?

```bash
# Linux
fusermount3 -u /path/to/mount

# Старые версии Linux
fusermount -u /path/to/mount

# macOS
umount /path/to/mount

# Принудительное размонтирование
sudo umount -f /path/to/mount
```

### Можно ли монтировать удаленные репозитории?

В текущей версии GitFS работает только с локальными репозиториями. Для работы с удаленными репозиториями сначала склонируйте их:

```bash
git clone https://github.com/user/repo.git
./gitfs repo /mount/point
```

### Можно ли монтировать конкретную ветку или коммит?

В текущей версии GitFS монтирует только HEAD коммит. Поддержка других коммитов планируется в будущих версиях.

Временное решение:

```bash
cd /path/to/repo
git checkout desired-branch
cd /path/to/gitfs
./gitfs /path/to/repo /mount/point
```

### Почему файлы доступны только для чтения?

GitFS предназначен для просмотра содержимого Git репозиториев, а не для их модификации. Это сделано для:

- Предотвращения случайного повреждения репозитория
- Обеспечения консистентности данных
- Упрощения реализации

## Производительность

### GitFS работает медленно на больших репозиториях

Попробуйте следующие оптимизации:

1. **Убедитесь, что кэширование включено**:
   ```bash
   ./gitfs repo mount -o kernel_cache
   ```

2. **Используйте SSD** для лучшей производительности I/O

3. **Ограничьте количество одновременных операций**

4. **Проверьте размер репозитория**:
   ```bash
   du -sh /path/to/repo/.git
   ```

### Высокое потребление памяти

GitFS кэширует данные для повышения производительности. Если потребление памяти критично:

1. **Перезапустите GitFS** для очистки кэша
2. **Уменьшите нагрузку** на файловую систему
3. **Мониторьте использование памяти**:
   ```bash
   ps aux | grep gitfs
   top -p $(pgrep gitfs)
   ```

## Ошибки и устранение неполадок

### "Transport endpoint is not connected"

Эта ошибка обычно означает, что GitFS процесс завершился неожиданно:

```bash
# Размонтирование
fusermount3 -u /mount/point

# Проверка процессов
ps aux | grep gitfs

# Принудительная очистка
sudo umount -f /mount/point
```

### "Device or resource busy"

Точка монтирования используется другим процессом:

```bash
# Найти процессы, использующие точку монтирования
lsof /mount/point
fuser -v /mount/point

# Завершить процессы
fuser -k /mount/point

# Размонтировать
fusermount3 -u /mount/point
```

### "Permission denied"

Проблемы с правами доступа:

```bash
# Проверить права на FUSE
ls -l /dev/fuse

# Добавить пользователя в группу fuse
sudo usermod -a -G fuse $USER

# Перелогиниться или выполнить
newgrp fuse

# Использовать allow_other
./gitfs repo mount -o allow_other
```

### "No such file or directory" для существующих файлов

Возможные причины:

1. **Файл не закоммичен в Git**:
   ```bash
   cd /path/to/repo
   git status
   git add missing-file
   git commit -m "Add missing file"
   ```

2. **Файл в другой ветке**:
   ```bash
   git branch -a
   git checkout correct-branch
   ```

3. **Проблемы с кэшем**:
   ```bash
   # Перезапустить GitFS
   fusermount3 -u /mount/point
   ./gitfs repo mount -f
   ```

### Утечки памяти

Если подозреваете утечки памяти:

```bash
# Проверка с valgrind
valgrind --leak-check=full ./gitfs repo mount -f

# Мониторинг памяти
watch -n 1 'ps aux | grep gitfs'
```

## Разработка и отладка

### Как включить отладочный вывод?

```bash
# Запуск в foreground с отладкой
./gitfs repo mount -f -d

# Сохранение логов
./gitfs repo mount -f 2> gitfs.log

# Фильтрация логов
./gitfs repo mount -f 2>&1 | grep "gitfs"
```

### Как добавить собственные логи?

```c
// В коде GitFS
fprintf(stderr, "[gitfs] debug: %s called with path=%s\n", __func__, path);
```

### Как запустить под отладчиком?

```bash
# Сборка с отладочной информацией
make CFLAGS="-g -O0 -Wall -Wextra -pedantic -DFUSE_USE_VERSION=31 -Iinclude $(PKG_CFLAGS)"

# Запуск под GDB
gdb --args ./gitfs repo mount -f

# В GDB
(gdb) break gitfs_getattr
(gdb) run
```

## Интеграция

### Как использовать GitFS в скриптах?

```bash
#!/bin/bash

# Функция для безопасного монтирования
mount_gitfs() {
    local repo=$1
    local mount=$2
    
    mkdir -p "$mount"
    ./gitfs "$repo" "$mount" &
    local pid=$!
    
    # Ждем монтирования
    for i in {1..30}; do
        if mountpoint -q "$mount"; then
            echo "Mounted successfully"
            return 0
        fi
        sleep 1
    done
    
    echo "Mount failed"
    kill $pid 2>/dev/null
    return 1
}

# Функция для безопасного размонтирования
umount_gitfs() {
    local mount=$1
    fusermount3 -u "$mount" || umount "$mount"
}

# Использование
mount_gitfs "/path/to/repo" "/tmp/mount"
# ... работа с файлами
umount_gitfs "/tmp/mount"
```

### Как интегрировать с systemd?

Создайте `/etc/systemd/system/gitfs@.service`:

```ini
[Unit]
Description=GitFS mount for %i
After=local-fs.target

[Service]
Type=forking
ExecStart=/usr/local/bin/gitfs /var/git/%i /mnt/gitfs/%i
ExecStop=/bin/fusermount3 -u /mnt/gitfs/%i
Restart=on-failure
User=gitfs
Group=gitfs

[Install]
WantedBy=multi-user.target
```

Использование:

```bash
# Запуск
sudo systemctl start gitfs@myrepo

# Автозапуск
sudo systemctl enable gitfs@myrepo
```

### Как использовать с Docker?

```dockerfile
FROM ubuntu:20.04

RUN apt-get update && apt-get install -y \
    libfuse3-dev \
    libgit2-dev \
    pkg-config \
    build-essential

COPY . /app
WORKDIR /app
RUN make

# Требует --privileged или --device /dev/fuse
CMD ["./gitfs", "/repo", "/mount", "-f"]
```

## Безопасность

### Безопасно ли использовать GitFS?

GitFS работает в режиме только для чтения и не может модифицировать Git репозиторий. Однако:

- Используйте `allow_other` только при необходимости
- Запускайте от непривилегированного пользователя
- Ограничивайте доступ к точкам монтирования

### Можно ли использовать в production?

GitFS находится в активной разработке. Для production использования:

- Тщательно протестируйте на вашей нагрузке
- Мониторьте производительность и стабильность
- Имейте план отката
- Рассмотрите альтернативы для критических систем

## Получение помощи

### Где сообщить об ошибке?

1. Проверьте [существующие issues](https://github.com/username/git_fuse_fs/issues)
2. Создайте [новый issue](https://github.com/username/git_fuse_fs/issues/new) с:
   - Описанием проблемы
   - Шагами для воспроизведения
   - Версией ОС и зависимостей
   - Логами GitFS

### Где задать вопрос?

- [GitHub Discussions](https://github.com/username/git_fuse_fs/discussions)
- [Issues](https://github.com/username/git_fuse_fs/issues) для багов
- [Stack Overflow](https://stackoverflow.com/questions/tagged/fuse) для общих вопросов о FUSE

### Как внести вклад?

См. [Руководство разработчика](Developer-Guide.md) для подробной информации о:

- Настройке среды разработки
- Стандартах кодирования
- Процессе создания Pull Request
- Тестировании изменений