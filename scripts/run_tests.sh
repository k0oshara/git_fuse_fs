#!/bin/bash

set -e

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Функции для вывода
print_header() {
    echo -e "${BLUE}================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

# Проверка зависимостей
check_dependencies() {
    print_header "Проверка зависимостей"
    
    local deps_ok=true
    
    # Проверка компилятора
    if command -v gcc >/dev/null 2>&1; then
        print_success "GCC найден: $(gcc --version | head -n1)"
    else
        print_error "GCC не найден"
        deps_ok=false
    fi
    
    # Проверка pkg-config
    if command -v pkg-config >/dev/null 2>&1; then
        print_success "pkg-config найден"
    else
        print_error "pkg-config не найден"
        deps_ok=false
    fi
    
    # Проверка FUSE
    if pkg-config --exists fuse3; then
        print_success "FUSE3 найден: $(pkg-config --modversion fuse3)"
    elif pkg-config --exists fuse; then
        print_success "FUSE найден: $(pkg-config --modversion fuse)"
    else
        print_error "FUSE не найден"
        deps_ok=false
    fi
    
    # Проверка libgit2
    if pkg-config --exists libgit2; then
        print_success "libgit2 найден: $(pkg-config --modversion libgit2)"
    else
        print_error "libgit2 не найден"
        deps_ok=false
    fi
    
    # Проверка FUSE устройства
    if [ -e /dev/fuse ]; then
        print_success "/dev/fuse доступен"
    else
        print_error "/dev/fuse недоступен"
        deps_ok=false
    fi
    
    if [ "$deps_ok" = false ]; then
        print_error "Некоторые зависимости отсутствуют. Установите их перед продолжением."
        exit 1
    fi
    
    echo
}

# Очистка предыдущих сборок
cleanup() {
    print_header "Очистка"
    
    make clean >/dev/null 2>&1 || true
    rm -rf test-repo test-mount test-mount-* >/dev/null 2>&1 || true
    
    # Принудительное размонтирование
    for mount in test-mount test-mount-*; do
        if mountpoint -q "$mount" 2>/dev/null; then
            print_info "Размонтирование $mount"
            fusermount3 -u "$mount" 2>/dev/null || fusermount -u "$mount" 2>/dev/null || umount "$mount" 2>/dev/null || true
        fi
    done
    
    print_success "Очистка завершена"
    echo
}

# Сборка проекта
build() {
    print_header "Сборка проекта"
    
    if make; then
        print_success "Сборка успешна"
    else
        print_error "Ошибка сборки"
        exit 1
    fi
    
    if [ -x "./gitfs" ]; then
        print_success "Исполняемый файл gitfs создан"
    else
        print_error "Исполняемый файл gitfs не найден"
        exit 1
    fi
    
    echo
}

# Проверка форматирования кода
check_formatting() {
    print_header "Проверка форматирования кода"
    
    if command -v clang-format >/dev/null 2>&1; then
        # Сохраняем текущее состояние
        git stash push -m "temp stash for formatting check" >/dev/null 2>&1 || true
        
        # Применяем форматирование
        make lint >/dev/null 2>&1
        
        # Проверяем изменения
        if git diff --quiet 2>/dev/null; then
            print_success "Код отформатирован правильно"
        else
            print_warning "Код требует форматирования. Запустите 'make lint'"
            git diff --name-only 2>/dev/null || true
        fi
        
        # Восстанавливаем состояние
        git stash pop >/dev/null 2>&1 || true
    else
        print_warning "clang-format не найден, пропускаем проверку форматирования"
    fi
    
    echo
}

# Создание тестового репозитория
create_test_repo() {
    print_header "Создание тестового репозитория"
    
    rm -rf test-repo
    mkdir test-repo
    cd test-repo
    
    git init >/dev/null
    git config user.email "test@example.com"
    git config user.name "Test User"
    
    # Создание тестовых файлов
    echo "Hello, GitFS!" > hello.txt
    echo "This is a test file with multiple lines." > multiline.txt
    echo "Line 2 of the test file." >> multiline.txt
    echo "Line 3 of the test file." >> multiline.txt
    
    mkdir -p subdir/nested
    echo "Nested file content" > subdir/nested.txt
    echo "Deeply nested content" > subdir/nested/deep.txt
    
    # Создание бинарного файла
    printf '\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F' > binary.dat
    
    # Создание файла с русскими символами
    echo "Привет, мир! 🌍" > unicode.txt
    
    git add . >/dev/null
    git commit -m "Initial test commit" >/dev/null
    
    cd ..
    
    print_success "Тестовый репозиторий создан"
    echo
}

# Базовые функциональные тесты
run_basic_tests() {
    print_header "Базовые функциональные тесты"
    
    mkdir -p test-mount
    
    # Запуск GitFS в фоне
    print_info "Запуск GitFS..."
    timeout 30s ./gitfs test-repo test-mount -f &
    local gitfs_pid=$!
    
    # Ожидание монтирования
    local mounted=false
    for i in {1..30}; do
        if [ -f test-mount/hello.txt ] 2>/dev/null; then
            mounted=true
            break
        fi
        sleep 0.5
    done
    
    if [ "$mounted" = false ]; then
        print_error "Не удалось смонтировать GitFS"
        kill $gitfs_pid 2>/dev/null || true
        return 1
    fi
    
    print_success "GitFS смонтирован"
    
    # Тест 1: Проверка существования файлов
    if [ -f test-mount/hello.txt ]; then
        print_success "Тест 1: Файл hello.txt существует"
    else
        print_error "Тест 1: Файл hello.txt не найден"
    fi
    
    # Тест 2: Чтение содержимого файла
    local content=$(cat test-mount/hello.txt 2>/dev/null)
    if [ "$content" = "Hello, GitFS!" ]; then
        print_success "Тест 2: Содержимое файла корректно"
    else
        print_error "Тест 2: Неверное содержимое файла: '$content'"
    fi
    
    # Тест 3: Проверка директорий
    if [ -d test-mount/subdir ]; then
        print_success "Тест 3: Директория subdir существует"
    else
        print_error "Тест 3: Директория subdir не найдена"
    fi
    
    # Тест 4: Вложенные файлы
    if [ -f test-mount/subdir/nested.txt ]; then
        print_success "Тест 4: Вложенный файл существует"
    else
        print_error "Тест 4: Вложенный файл не найден"
    fi
    
    # Тест 5: Глубоко вложенные файлы
    if [ -f test-mount/subdir/nested/deep.txt ]; then
        print_success "Тест 5: Глубоко вложенный файл существует"
    else
        print_error "Тест 5: Глубоко вложенный файл не найден"
    fi
    
    # Тест 6: Бинарные файлы
    if [ -f test-mount/binary.dat ]; then
        local size=$(stat -c%s test-mount/binary.dat 2>/dev/null)
        if [ "$size" = "16" ]; then
            print_success "Тест 6: Бинарный файл корректен"
        else
            print_error "Тест 6: Неверный размер бинарного файла: $size"
        fi
    else
        print_error "Тест 6: Бинарный файл не найден"
    fi
    
    # Тест 7: Unicode файлы
    if [ -f test-mount/unicode.txt ]; then
        print_success "Тест 7: Unicode файл существует"
    else
        print_error "Тест 7: Unicode файл не найден"
    fi
    
    # Тест 8: Листинг директории
    local file_count=$(ls test-mount/ 2>/dev/null | wc -l)
    if [ "$file_count" -ge "5" ]; then
        print_success "Тест 8: Листинг директории работает ($file_count файлов)"
    else
        print_error "Тест 8: Неверное количество файлов в листинге: $file_count"
    fi
    
    # Тест 9: Права доступа (только чтение)
    if touch test-mount/newfile.txt 2>/dev/null; then
        print_error "Тест 9: Файловая система не защищена от записи"
        rm -f test-mount/newfile.txt 2>/dev/null || true
    else
        print_success "Тест 9: Файловая система защищена от записи"
    fi
    
    # Завершение GitFS
    kill $gitfs_pid 2>/dev/null || true
    wait $gitfs_pid 2>/dev/null || true
    
    # Размонтирование
    fusermount3 -u test-mount 2>/dev/null || fusermount -u test-mount 2>/dev/null || umount test-mount 2>/dev/null || true
    sleep 1
    
    print_success "Базовые тесты завершены"
    echo
}

# Запуск автоматических тестов
run_automated_tests() {
    print_header "Автоматические тесты"
    
    if make test; then
        print_success "Автоматические тесты прошли успешно"
    else
        print_error "Автоматические тесты завершились с ошибкой"
        return 1
    fi
    
    echo
}

# Тесты производительности
run_performance_tests() {
    print_header "Тесты производительности"
    
    # Создание большого репозитория
    print_info "Создание большого тестового репозитория..."
    rm -rf big-test-repo
    mkdir big-test-repo
    cd big-test-repo
    
    git init >/dev/null
    git config user.email "test@example.com"
    git config user.name "Test User"
    
    # Создание множества файлов
    for i in {1..100}; do
        echo "File $i content with some text to make it non-empty" > "file_$i.txt"
    done
    
    # Создание вложенных директорий
    for i in {1..10}; do
        mkdir -p "dir_$i/subdir_$i"
        for j in {1..10}; do
            echo "Nested file $i-$j" > "dir_$i/subdir_$i/nested_$j.txt"
        done
    done
    
    git add . >/dev/null
    git commit -m "Big repo commit" >/dev/null
    cd ..
    
    mkdir -p big-test-mount
    
    # Тест производительности монтирования
    print_info "Тестирование времени монтирования..."
    local start_time=$(date +%s.%N)
    
    timeout 60s ./gitfs big-test-repo big-test-mount -f &
    local gitfs_pid=$!
    
    # Ожидание монтирования
    local mounted=false
    for i in {1..60}; do
        if [ -f big-test-mount/file_1.txt ] 2>/dev/null; then
            mounted=true
            break
        fi
        sleep 0.5
    done
    
    local end_time=$(date +%s.%N)
    local mount_time=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "N/A")
    
    if [ "$mounted" = true ]; then
        print_success "Большой репозиторий смонтирован за ${mount_time}s"
        
        # Тест производительности листинга
        print_info "Тестирование производительности листинга..."
        local start_time=$(date +%s.%N)
        local file_count=$(ls big-test-mount/ 2>/dev/null | wc -l)
        local end_time=$(date +%s.%N)
        local list_time=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "N/A")
        
        print_success "Листинг $file_count файлов за ${list_time}s"
        
        # Тест производительности чтения
        print_info "Тестирование производительности чтения..."
        local start_time=$(date +%s.%N)
        for i in {1..10}; do
            cat big-test-mount/file_$i.txt >/dev/null 2>&1
        done
        local end_time=$(date +%s.%N)
        local read_time=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "N/A")
        
        print_success "Чтение 10 файлов за ${read_time}s"
    else
        print_error "Не удалось смонтировать большой репозиторий"
    fi
    
    # Очистка
    kill $gitfs_pid 2>/dev/null || true
    wait $gitfs_pid 2>/dev/null || true
    fusermount3 -u big-test-mount 2>/dev/null || fusermount -u big-test-mount 2>/dev/null || umount big-test-mount 2>/dev/null || true
    rm -rf big-test-repo big-test-mount
    
    echo
}

# Проверка утечек памяти
check_memory_leaks() {
    print_header "Проверка утечек памяти"
    
    if ! command -v valgrind >/dev/null 2>&1; then
        print_warning "Valgrind не найден, пропускаем проверку утечек памяти"
        echo
        return
    fi
    
    mkdir -p test-mount-valgrind
    
    print_info "Запуск GitFS под Valgrind..."
    timeout 20s valgrind --leak-check=full --error-exitcode=1 --quiet \
        ./gitfs test-repo test-mount-valgrind -f &
    local valgrind_pid=$!
    
    # Ожидание монтирования
    sleep 3
    
    # Простые операции для проверки
    ls test-mount-valgrind/ >/dev/null 2>&1 || true
    cat test-mount-valgrind/hello.txt >/dev/null 2>&1 || true
    
    # Завершение
    kill $valgrind_pid 2>/dev/null || true
    
    if wait $valgrind_pid 2>/dev/null; then
        print_success "Утечки памяти не обнаружены"
    else
        local exit_code=$?
        if [ $exit_code -eq 1 ]; then
            print_error "Обнаружены утечки памяти"
        else
            print_warning "Valgrind завершился с кодом $exit_code"
        fi
    fi
    
    fusermount3 -u test-mount-valgrind 2>/dev/null || fusermount -u test-mount-valgrind 2>/dev/null || umount test-mount-valgrind 2>/dev/null || true
    rm -rf test-mount-valgrind
    
    echo
}

# Основная функция
main() {
    print_header "GitFS Test Suite"
    echo "Автоматизированное тестирование GitFS"
    echo
    
    # Проверка аргументов
    local run_all=true
    local run_basic=false
    local run_auto=false
    local run_perf=false
    local run_memory=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --basic)
                run_all=false
                run_basic=true
                shift
                ;;
            --auto)
                run_all=false
                run_auto=true
                shift
                ;;
            --performance)
                run_all=false
                run_perf=true
                shift
                ;;
            --memory)
                run_all=false
                run_memory=true
                shift
                ;;
            --help)
                echo "Использование: $0 [опции]"
                echo "Опции:"
                echo "  --basic       Только базовые тесты"
                echo "  --auto        Только автоматические тесты"
                echo "  --performance Только тесты производительности"
                echo "  --memory      Только проверка утечек памяти"
                echo "  --help        Показать эту справку"
                echo
                echo "Без опций запускаются все тесты"
                exit 0
                ;;
            *)
                print_error "Неизвестная опция: $1"
                exit 1
                ;;
        esac
    done
    
    # Выполнение тестов
    check_dependencies
    cleanup
    build
    check_formatting
    create_test_repo
    
    local tests_failed=0
    
    if [ "$run_all" = true ] || [ "$run_basic" = true ]; then
        if ! run_basic_tests; then
            tests_failed=$((tests_failed + 1))
        fi
    fi
    
    if [ "$run_all" = true ] || [ "$run_auto" = true ]; then
        if ! run_automated_tests; then
            tests_failed=$((tests_failed + 1))
        fi
    fi
    
    if [ "$run_all" = true ] || [ "$run_perf" = true ]; then
        if ! run_performance_tests; then
            tests_failed=$((tests_failed + 1))
        fi
    fi
    
    if [ "$run_all" = true ] || [ "$run_memory" = true ]; then
        check_memory_leaks
    fi
    
    # Финальная очистка
    cleanup
    
    # Результаты
    print_header "Результаты тестирования"
    
    if [ $tests_failed -eq 0 ]; then
        print_success "Все тесты прошли успешно! 🎉"
        exit 0
    else
        print_error "Некоторые тесты завершились с ошибкой ($tests_failed)"
        exit 1
    fi
}

# Запуск основной функции
main "$@"
