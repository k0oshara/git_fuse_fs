# Git_fuse_fs

```
# Запустить gitfs в фоновом режиме (демон, поведение по умолчанию)
./gitfs /path/to/repo /path/to/mount -o ro,allow_other
```

```
# Запустить gitfs в foreground (не форкаться), чтобы видеть логи в терминале
./gitfs /path/to/repo /path/to/mount -f -o ro,allow_other
```