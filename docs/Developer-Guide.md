# Руководство разработчика

Это руководство предназначено для разработчиков, которые хотят внести вклад в GitFS или понять его внутреннее устройство.

## Архитектура проекта

### Структура файлов

```
git_fuse_fs/
├── main.c              # Точка входа приложения
├── include/
│   ├── gitfs.h         # Основные структуры и объявления
│   └── uthash.h        # Библиотека хеш-таблиц (загружается автоматически)
├── src/
│   └── gitfs.c         # Реализация FUSE операций
├── test/
│   └── test_gitfs.c    # Тестовая утилита
├── docs/               # Документация
├── .github/workflows/  # CI/CD конфигурация
├── Makefile           # Сборочный скрипт
└── README.md          # Основная документация
```

### Основные компоненты

#### 1. Структуры данных

```c
// Основное состояние GitFS
struct gitfs_state {
    git_repository *repo;  // Указатель на Git репозиторий
    git_commit *commit;    // Текущий коммит (HEAD)
};

// Кэш путей к объектам
struct path_cache_entry {
    char *path;           // Путь в файловой системе
    git_oid oid;          // Git OID объекта
    git_object_t type;    // Тип объекта (tree/blob)
    UT_hash_handle hh;    // Хеш-таблица uthash
};

// Кэш содержимого файлов
struct blob_cache_entry {
    char oidhex[GIT_OID_HEXSZ + 1];  // OID в hex формате
    git_blob *blob;                   // Указатель на blob объект
    UT_hash_handle hh;                // Хеш-таблица uthash
};
```

#### 2. FUSE операции

GitFS реализует следующие FUSE операции:

- [`gitfs_init()`](../src/gitfs.c:185) - инициализация файловой системы
- [`gitfs_destroy()`](../src/gitfs.c:192) - очистка ресурсов
- [`gitfs_getattr()`](../src/gitfs.c:204) - получение атрибутов файлов/директорий
- [`gitfs_readdir()`](../src/gitfs.c:232) - чтение содержимого директорий
- [`gitfs_open()`](../src/gitfs.c:261) - открытие файлов
- [`gitfs_read()`](../src/gitfs.c:271) - чтение содержимого файлов

#### 3. Кэширование

GitFS использует двухуровневое кэширование:

1. **Path Cache** - кэширует соответствие путей файловой системы Git объектам
2. **Blob Cache** - кэширует содержимое файлов для избежания повторных обращений к Git

## Настройка среды разработки

### Зависимости для разработки

```bash
# Ubuntu/Debian
sudo apt-get install \
    libfuse3-dev \
    libgit2-dev \
    pkg-config \
    build-essential \
    clang-format \
    valgrind \
    gdb \
    git

# macOS
brew install \
    libgit2 \
    macfuse \
    pkg-config \
    clang-format \
    valgrind
```

### Настройка IDE

#### VS Code

Создайте `.vscode/settings.json`:

```json
{
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/include",
        "/usr/include/fuse3",
        "/usr/include/git2"
    ],
    "C_Cpp.default.defines": [
        "FUSE_USE_VERSION=31"
    ],
    "C_Cpp.default.compilerPath": "/usr/bin/gcc",
    "C_Cpp.default.cStandard": "c11"
}
```

Создайте `.vscode/tasks.json`:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "build",
            "type": "shell",
            "command": "make",
            "group": {
                "kind": "build",
                "isDefault": true
            }
        },
        {
            "label": "test",
            "type": "shell",
            "command": "make test",
            "group": "test"
        },
        {
            "label": "clean",
            "type": "shell",
            "command": "make clean"
        }
    ]
}
```

#### CLion

Создайте `CMakeLists.txt` для CLion:

```cmake
cmake_minimum_required(VERSION 3.10)
project(gitfs)

set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE3 REQUIRED fuse3)
pkg_check_modules(LIBGIT2 REQUIRED libgit2)

add_definitions(-DFUSE_USE_VERSION=31)

include_directories(include)
include_directories(${FUSE3_INCLUDE_DIRS})
include_directories(${LIBGIT2_INCLUDE_DIRS})

add_executable(gitfs main.c src/gitfs.c)
target_link_libraries(gitfs ${FUSE3_LIBRARIES} ${LIBGIT2_LIBRARIES})

add_executable(test_gitfs test/test_gitfs.c)
target_link_libraries(test_gitfs ${FUSE3_LIBRARIES} ${LIBGIT2_LIBRARIES})
```

## Процесс разработки

### Workflow

1. **Fork** репозитория
2. **Clone** вашего fork
3. Создайте **feature branch**: `git checkout -b feature/amazing-feature`
4. Внесите изменения
5. **Тестирование**: `make test`
6. **Форматирование**: `make lint`
7. **Commit**: `git commit -m 'Add amazing feature'`
8. **Push**: `git push origin feature/amazing-feature`
9. Создайте **Pull Request**

### Стандарты кодирования

#### Стиль кода

Проект использует `.clang-format` для автоматического форматирования:

```bash
# Форматирование всех файлов
make lint

# Форматирование конкретного файла
clang-format -i --style=file src/gitfs.c
```

#### Соглашения об именовании

- **Функции**: `gitfs_function_name()`
- **Структуры**: `struct gitfs_structure_name`
- **Константы**: `GITFS_CONSTANT_NAME`
- **Переменные**: `snake_case`

#### Комментарии

```c
/**
 * Краткое описание функции
 * 
 * @param param1 Описание параметра
 * @param param2 Описание параметра
 * @return Описание возвращаемого значения
 */
int gitfs_function(int param1, const char *param2);

// Однострочный комментарий для простых случаев
int simple_var = 0;
```

### Отладка

#### GDB

```bash
# Сборка с отладочной информацией
make CFLAGS="-g -O0 -Wall -Wextra -pedantic -DFUSE_USE_VERSION=31 -Iinclude $(PKG_CFLAGS)"

# Запуск под отладчиком
gdb --args ./gitfs test-repo test-mount -f

# В GDB
(gdb) break gitfs_getattr
(gdb) run
(gdb) continue
(gdb) print *stbuf
(gdb) backtrace
```

#### Valgrind

```bash
# Проверка утечек памяти
valgrind --leak-check=full --show-leak-kinds=all ./gitfs test-repo test-mount -f

# Проверка ошибок доступа к памяти
valgrind --tool=memcheck ./gitfs test-repo test-mount -f
```

#### Логирование

GitFS выводит отладочную информацию в stderr:

```c
// Добавление отладочного вывода
fprintf(stderr, "[gitfs] debug: operation=%s path=%s\n", __func__, path);
```

```bash
# Сохранение логов
./gitfs test-repo test-mount -f 2> debug.log

# Фильтрация логов
./gitfs test-repo test-mount -f 2>&1 | grep "gitfs"
```

## Тестирование

### Автоматические тесты

```bash
# Запуск всех тестов
make test

# Сборка только тестов
make test/test_gitfs

# Запуск с подробным выводом
cd test && ./test_gitfs
```

### Ручное тестирование

```bash
# Создание тестового репозитория
mkdir test-repo && cd test-repo
git init
git config user.email "test@example.com"
git config user.name "Test User"
echo "content" > file.txt
git add . && git commit -m "test"
cd ..

# Тестирование различных сценариев
mkdir test-mount
./gitfs test-repo test-mount -f &

# В другом терминале
ls test-mount/
cat test-mount/file.txt
stat test-mount/file.txt
find test-mount -type f
```

### Тестирование производительности

```bash
# Создание большого репозитория
mkdir big-repo && cd big-repo
git init
for i in {1..1000}; do
    echo "File $i content" > "file_$i.txt"
done
git add . && git commit -m "1000 files"
cd ..

# Тестирование производительности
time ls big-repo-mount/
time find big-repo-mount -name "*.txt" | wc -l
```

## Добавление новых функций

### Пример: Добавление поддержки символических ссылок

1. **Обновите структуры данных**:

```c
// В gitfs.h
int gitfs_readlink(const char *path, char *buf, size_t size);
```

2. **Реализуйте функцию**:

```c
// В gitfs.c
int gitfs_readlink(const char *path, char *buf, size_t size) {
    // Реализация чтения символической ссылки
    return 0;
}
```

3. **Добавьте в операции FUSE**:

```c
struct fuse_operations gitfs_oper = {
    // ... существующие операции
    .readlink = gitfs_readlink,
};
```

4. **Добавьте тесты**:

```c
void test_symlink(void) {
    // Тест символических ссылок
}
```

### Пример: Добавление поддержки других коммитов

1. **Обновите структуру состояния**:

```c
struct gitfs_state {
    git_repository *repo;
    git_commit *commit;
    char *revision;  // Новое поле для ревизии
};
```

2. **Обновите инициализацию**:

```c
int gitfs_init_repo(struct gitfs_state *st, const char *repo_path, const char *rev) {
    // Сохранение ревизии
    st->revision = strdup(rev);
    // ... остальная логика
}
```

## Оптимизация производительности

### Профилирование

```bash
# Использование perf (Linux)
perf record ./gitfs test-repo test-mount -f
perf report

# Использование gprof
gcc -pg -o gitfs main.c src/gitfs.c $(PKG_CFLAGS) $(PKG_LIBS)
./gitfs test-repo test-mount -f
gprof gitfs gmon.out > analysis.txt
```

### Оптимизация кэша

```c
// Настройка размера кэша
#define MAX_CACHE_ENTRIES 10000

// Очистка старых записей кэша
void cleanup_old_cache_entries(void) {
    // Реализация LRU или другого алгоритма
}
```

## Contributing

### Checklist для Pull Request

- [ ] Код соответствует стилю проекта (`make lint`)
- [ ] Все тесты проходят (`make test`)
- [ ] Добавлены тесты для новой функциональности
- [ ] Обновлена документация
- [ ] Нет утечек памяти (проверено valgrind)
- [ ] Commit сообщения информативны

### Типы изменений

- **feat**: новая функциональность
- **fix**: исправление ошибки
- **docs**: изменения в документации
- **style**: форматирование кода
- **refactor**: рефакторинг без изменения функциональности
- **test**: добавление или изменение тестов
- **chore**: изменения в сборке или вспомогательных инструментах

### Пример commit сообщения

```
feat: add support for symbolic links

- Implement gitfs_readlink() function
- Add symlink detection in gitfs_getattr()
- Update FUSE operations structure
- Add tests for symlink functionality

Closes #123
```

## Полезные ресурсы

### Документация

- [FUSE Documentation](https://libfuse.github.io/doxygen/)
- [libgit2 API Reference](https://libgit2.org/libgit2/)
- [uthash Documentation](https://troydhanson.github.io/uthash/)

### Примеры FUSE файловых систем

- [hello.c](https://github.com/libfuse/libfuse/blob/master/example/hello.c) - простой пример
- [passthrough.c](https://github.com/libfuse/libfuse/blob/master/example/passthrough.c) - проксирующая ФС
- [memfs](https://github.com/bbonev/memfs) - файловая система в памяти

### Инструменты разработки

- [strace](https://strace.io/) - трассировка системных вызовов
- [ltrace](https://ltrace.org/) - трассировка библиотечных вызовов
- [fusermount3](https://github.com/libfuse/libfuse) - утилита монтирования FUSE