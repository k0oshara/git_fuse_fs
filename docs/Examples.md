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
