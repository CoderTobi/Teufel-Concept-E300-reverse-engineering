# Aufbau Subwoofer
Der Subwoofer hat vier Platinen an seiner Rückseite. 

## 5V Standby Netzteil und Relay
Die kleinste Platine ist ein 5V Netzteil welches das Frontpanel bzw. Display Board mit Strom versorgt. 
Das Netzteil ist dauerhaft aktiv, solange der Hauptschalter aktiviert ist.
Es hat außerdem ein Relay, welches den großen Transformator mit 230V Wechselspannung versorgt, sobald das Frontpanel den Standby Pin `ST` auf `high` setzt.

## Transformator
Im unteren Teil des Subwoofer gibt es einen sehr großen Trafo, welcher die 230V 50Hz Wechselspannung in zwei andere Wechselspannungen transformiert.
Eine Ausgang geht an den Subwoofer AMP, der andere an den Satelliten AMP.
### Anschlüsse
#### Standby Netzteil
- 2 x dicke Rote Kabel 230V 50Hz
#### Subwoofer AMP
- Dickes Blaues Kabel
- Dickes Schwarzes Kabel
- Dickes Blaues Kabel
#### Satelliten AMP
- Dickes Gelbes Kabel
- Dickes Schwarzes Kabel
- Dickes Gelbes Kabel

# Display Board bzw. Frontpanel
Teufel bezeichnet diese Platine als Display Board, obwohl es keinerlei Displays besitzt.
Daher bezeichne ich es auch gerne als Frontpanel.
Es beherbergt einen Pushbutton (Standby Knopf) und zwei Potentiometer (Bass und Volume Regler).
Außerdem befindet sich dort ein IC und ein 7-Pin JST-HX Anschluss, über welchen es mit dem IO Board verbunden ist.
Siehe auch: [Anschluss Display Board](Anschluss_Display_Board.md)

## IO Board
Am IO Board befinden sich die ganzen äußeren Anschlüsse des Subwoofer.
Dort befindet sich auch der Anschluss des Frontpanels bzw. Display Boards.
Manchmal bezeichne ich es auf Grund seiner Position auch als Mainboard.
Das Board scheint nicht wirklich viel Logik zu beherbergen, obwohl es einen IC hat.
Das `MUTE` Signal vom Frontpanel wird hier an die beiden AMPs durchgereicht.
Das `ST` Signal vom Frontpanel wird ebenfalls einfach an das Relay des Netzteils durchgereicht.
### Beschriftungen:
- VER: 0.3
- BA0802-01
- CONCEPT E3/E4 SUB_1#PCB
- Layer 2 SIZE: 131mm...
### Anschlüsse:
#### Frontpanel (JST-HX)
- +5V
- GND
- C-ST
- MUTE
- SCL
- SDA
- C-D
#### Standby Netzteil
- C-ST
- GND
- +5V
#### Subwoofer AMP
- SW_SI
- GND
- +15V
- GND
- -15V
- MUTE
#### Satelliten AMP
- FL
- FR
- SL
- SR
- CEN
- MUTE
- GND
#### Satelliten AMP Output
- FL
- FR
- RL
- RR
- CEN
- GND

## Subwoofer AMP
Das `MUTE` Signal des Frontpanels wird hier genutzt, um ein weiteres Relay zu schalten.
Wenn `MUTE` auf `high` wechselt schält das Relay durch und der Subwoofer macht Töne.
Bei `low` ist er stumm geschaltet.
Da er zwei große 10.000 Micro Farad 50V Kondensatoren besitzt wird er erst nach einer gewissen Zeit vom Frontpanel unmuted.
Ansonsten bekommt das Board seinen Strom vom Trafo und sein Audio Signal vom IO Board.
### Beschriftungen:
- VER: 0.3
- BA0801-04
- CONCEPT E3/E4/EXTREME
- SIZE: 150mmx86mm
### Anschlüsse:
#### IO Board
- SIGNAL
- GND
- +15V
- GND
- -15V
- CONTROL

## Satelliten AMP
Dieser AMP ist ebenfalls stumm, bis `MUTE` vom Frontpanel auf `high` gesetzt wird.
Er besitzt jedoch kein eigenes Relay. 
Jedoch haben seine Kondensatoren auch nur eine Spannungsfestigkeit von 35V und nicht 50V.
### Beschriftungen:
- VER: 2.5
- MAGNUM BASIC | SUB_3#PCB
- 2009/3/3