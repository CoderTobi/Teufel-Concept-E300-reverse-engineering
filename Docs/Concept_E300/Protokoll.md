# Protokoll
Das Display Board kommuniziert mit dem Restlichen System via I2C. 
Siehe für weitere Informationen auch [Aufbau](Aufbau.md) und [Anschluss Display Board](Anschluss_Display_Board.md).

## Aufbau eines Pakets
TODO

## Einschalten
Beim Einschalten wird zuerst `ST` auf `low` gesetzt.
Dann ca. 2,5 Sekunden gewartet.
Anschließend werden über ``I2C`` Daten ausgetauscht. (TODO)
Dann nach 0,35 Sekunden `MUTE`auf `high` gesetzt.

> [!CAUTION]
> `MUTE` darf erst auf `high` gesetzt werden, nachdem `ST` ca. 3 Sekunden auf `low` war.

> [!WARNING]
> Sollten die AMPs via I2C keine Informationen über die gewünschte Lautstärke erhalten, kann es zu unerwartetem Verhalten kommen.
> Auf Grund eines Wackelkontakts hatte ich beispielsweise schon den Fall, dass einer der Satelliten Lautsprecher extrem laut war.

## Ausschalten
Zuerst werden Daten via ``I2C`` ausgetauscht. (TODO)
Dann `MUTE` wird auf `low` gesetzt.
10 Mikro Sekunden später dann `ST` auf `high`.
