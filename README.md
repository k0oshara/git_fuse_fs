# GitFS - Git Repository FUSE Filesystem

GitFS - это файловая система FUSE, которая позволяет монтировать Git репозитории в режиме только для чтения. Вы можете просматривать содержимое любого коммита как обычную файловую систему.

## Возможности

- 🔍 Просмотр содержимого Git репозитория как файловой системы
- 📁 Поддержка директорий и вложенных файлов
- 🔒 Режим только для чтения (read-only)
- ⚡ Кэширование для повышения производительности
- 🎯 Поддержка любых коммитов и веток
- 🐧 Кроссплатформенность (Linux, macOS)

## Требования

### Linux
```bash
sudo apt-get install libfuse3-dev libgit2-dev pkg-config
```

## Сборка

```bash
# Клонирование репозитория
git clone https://github.com/username/git_fuse_fs.git
cd git_fuse_fs

# Сборка
make

# Запуск тестов
make test
```

## Использование

### Базовое использование

```bash
# Монтирование репозитория (демон режим)
./gitfs /path/to/repo /path/to/mount -o ro,allow_other

# Просмотр файлов
ls /path/to/mount
cat /path/to/mount/README.md

# Размонтирование
fusermount3 -u /path/to/mount
```

### Режим отладки

```bash
# Запуск в foreground режиме для просмотра логов
./gitfs /path/to/repo /path/to/mount -f -o ro,allow_other
```

### Дополнительные опции FUSE

```bash
# Разрешить доступ другим пользователям
./gitfs /path/to/repo /path/to/mount -o ro,allow_other

# Автоматическое размонтирование при завершении процесса
./gitfs /path/to/repo /path/to/mount -o ro,auto_unmount

# Комбинирование опций
./gitfs /path/to/repo /path/to/mount -f -o ro,allow_other,auto_unmount
```

## Архитектура

GitFS состоит из следующих компонентов:

- **main.c** - точка входа и инициализация FUSE
- **src/gitfs.c** - основная логика файловой системы
- **include/gitfs.h** - заголовочные файлы и структуры данных

### Основные функции FUSE

- `gitfs_getattr()` - получение атрибутов файлов/директорий
- `gitfs_readdir()` - чтение содержимого директорий
- `gitfs_open()` - открытие файлов
- `gitfs_read()` - чтение содержимого файлов

### Кэширование

GitFS использует два уровня кэширования:

1. **Path Cache** - кэширование путей к объектам Git
2. **Blob Cache** - кэширование содержимого файлов

## Разработка

### Форматирование кода

```bash
make lint
```

### Запуск тестов

```bash
# Сборка и запуск всех тестов
make test

# Ручной запуск тестовой утилиты
cd test
./test_gitfs
```

### Отладка

```bash
# Сборка с отладочной информацией
make CFLAGS="-g -O0 -Wall -Wextra -pedantic -DFUSE_USE_VERSION=31 -Iinclude $(PKG_CFLAGS)"

# Запуск под отладчиком
gdb --args ./gitfs /path/to/repo /path/to/mount -f

# Проверка утечек памяти
valgrind --leak-check=full ./gitfs /path/to/repo /path/to/mount -f
```

## Устранение неполадок

### Проблемы с монтированием

```bash
# Проверка, что FUSE доступен
ls /dev/fuse

# Проверка прав доступа
groups | grep fuse

# Принудительное размонтирование
sudo fusermount3 -u /path/to/mount
```

### Проблемы с производительностью

- Убедитесь, что включено кэширование ядра (`kernel_cache = 1`)
- Проверьте размер репозитория и количество файлов
- Используйте SSD для лучшей производительности

### Логирование

GitFS выводит отладочную информацию в stderr при запуске с флагом `-f`:

```bash
./gitfs /path/to/repo /path/to/mount -f 2> gitfs.log
```

## Ограничения

- Только режим чтения (read-only)
- Монтируется только HEAD коммит (планируется поддержка других коммитов)
- Не поддерживаются символические ссылки
- Не поддерживаются расширенные атрибуты файлов

## Лицензия

Этот проект распространяется под лицензией, указанной в файле [LICENSE](LICENSE).

## Вклад в проект

1. Форкните репозиторий
2. Создайте ветку для новой функции (`git checkout -b feature/amazing-feature`)
3. Зафиксируйте изменения (`git commit -m 'Add amazing feature'`)
4. Отправьте в ветку (`git push origin feature/amazing-feature`)
5. Откройте Pull Request

## Связанные проекты

- [FUSE](https://github.com/libfuse/libfuse) - Filesystem in Userspace
- [libgit2](https://libgit2.org/) - Portable Git core library
- [uthash](https://troydhanson.github.io/uthash/) - Hash table for C structures

## Авторы

- Иванов Максим Сергеевич, Исаенков Александр Дмитриевич - [@k0oshara](https://github.com/k0oshara), [@gugukukua](https://github.com/PlayingPeano?tab=repositories)
