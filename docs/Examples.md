# Примеры использования GitFS

Этот документ содержит практические примеры использования GitFS в различных сценариях.

## Базовые примеры

### Монтирование в фоновом режиме

```bash
make clean && make

# Создаём точку монтирования
mkdir -p ~/git-mount

# Монтирование в демон режиме
./gitfs /path/to/repo ~/git-mount -o ro,allow_other

# Проверка монтирования
mount | grep gitfs

# Работа с файлами
ls ~/git-mount/
cat ~/git-mount/src/main.c

# Размонтирование
fusermount3 -u ~/git-mount
```

```bash
make clean && make

# Создаём точку монтирования
mkdir -p ~/git-mount

# Монтирование репозитория в foreground (-f), чтобы видеть логи
./gitfs /path/to/your/repo ~/git-mount -f -o ro,allow_other

# В другом терминале:
ls ~/git-mount/ 
cat ~/git-mount/README.md
find ~/git-mount -name "*.c"

# Чтобы остановить и демонтировать:
# просто нажмите Ctrl+C в терминале с gitfs
```
