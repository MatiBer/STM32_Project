# Projekt_STM32

![image](https://github.com/MatiBer/STM32_Project/assets/106385056/7c8d9fab-c855-40bc-b431-12d8abd07c82)


## Opis

Tematem projektu jest zegar. W normalnej pracy wyświetla aktualną godzinę i datę. Można go przełączyć na wyświetlanie temperatury. Sterowanie odbywa się przez pilot podczerwieni. Dane pokazywane są na wyswietlaczu tft przy pomocy SPI. Na zegarze można ustawiać date, godzinę i alarmy. Alarmy włączają brzęczyk, a na ekranie pojawia się komunikat.

## Wykorzystane modułey

- płytka NUCLEO-L476RG
- wyświetlacz tft ST7735s
- czujnik temperatury DS18B20
- Pilot IR (NEC)
- odbiornik podczerwieni VS1838B

## Schemat blokowy

![image](https://github.com/MatiBer/STM32_Project/assets/106385056/632b1335-b68b-4ade-b3a0-8a2d2ab49a5b)
