# Примеры использования GitFS

Этот документ содержит практические примеры использования GitFS в различных сценариях.

## Базовые примеры

### Монтирование в фоновом режиме

```bash
make clean && make

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
