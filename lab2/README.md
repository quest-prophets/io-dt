# Лабораторная работа 2

**Название:** "Разработка драйверов блочных устройств"

**Цель работы:** Получить знания и навыки разработки драйверов блочных устройств для операционной системы Linux.

## Описание функциональности драйвера

Драйвер создает виртуальный жесткий диск на 50 Мбайт и разбивает его на три первичных раздела по 10, 25 и 15 Мбайт соответственно.

## Инструкция по сборке

Запустить виртуальную машину с ядром нужной версии (4.15):

```sh
vagrant up
vagrant ssh
cd /vagrant
```

Собрать модуль драйвера:

```sh
make
```

## Инструкция пользователя

Установить модуль драйвера:

```sh
sudo insmod lab2.ko
```

Отформатировать разделы в FAT32:

```sh
for p in `seq 3`; do sudo mkfs -t vfat /dev/lbdv18p$p; done
```

## Примеры использования

Установка драйвера и форматирование диска:
![image](https://user-images.githubusercontent.com/26933429/110926778-dda4c300-8335-11eb-81da-6e18b7b2d771.png)


Измерение скорости обмена данными:

```sh
for bs in 512 2048 4096 16384 65536; do
  for i in `seq 10`; do
    sudo dd if=/dev/vda1 of=/dev/lbdv18p2 bs=$bs count=$(( 15*1024*1024/$bs )) oflag=direct 2>&1 \
      | grep copied \
      | perl -ne 'if (/([0-9.]+) MB\/s/) { print "$1\n" } elsif (/([0-9.]+) GB\/s/) { print (($1*1024)."\n") }';
  done | awk '{ total += $1; count++ } END { print "block size '$bs': " count " runs " total/count " MB/s avg" }';
done
```

Просмотр MBR для диска, разделенного на primary и extended разделы:

```sh
export D = /dev/lbdv18
sudo parted $D mklabel msdos;
sudo parted $D mkpart primary 1s $(( 10*1024*1024/512 ))s;
sudo parted $D mkpart extended $(( 10*1024*1024/512 + 1 ))s $(( 50*1024*1024/512 - 1 ))s;
sudo parted $D mkpart logical $(( 10*1024*1024/512 + 2 ))s $(( 25*1024*1024/512 ))s;
sudo parted $D mkpart logical $(( 25*1024*1024/512 + 2 ))s $(( 50*1024*1024/512 - 1 ))s;

sudo rmmod lab2.ko
dmesg | tail -n 120
```

## Тестирование производительности

| Размер блоков (байт) | Скорость при LBDV -> LBDV (Мб/с) | Скорость при VDA -> LBDV (Мб/с) |
| ------------- | ------------- | --- |
| 512	| 54.46	| 53.73 |
| 2048	| 187.3	| 219.3 |
| 4096	| 313.9	| 413.8 |
| 16384	| 519.9	| 775.1 |
| 65536	| 687.8 |	1219.08 |

![image](https://user-images.githubusercontent.com/26933429/110925439-3e330080-8334-11eb-82ab-63fafcfe26c2.png)
