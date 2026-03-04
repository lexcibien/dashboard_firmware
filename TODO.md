# TODO List

## Programação

- [ ] Comunicação com o painel
  - [ ] Pegar endereços CAN

## Hardware

- [ ] Adicionar display 14 segmentos
  - [ ] Usar um multiplexador 595 para reduzir quantidade de pinos necessários
  - [ ] Mostrar marchas
  - [ ] Substituir pelo display oLED?
- [ ] Testar o microcontrolador com módulo de rede CAN no painel
- [ ] Adicionar display redondo TFT
  - [ ] Mostrar mapa ou direções
  - [ ] Usar biblioteca TFT_eSPI ou squareline studio LVGL
  - [ ] Arrumar a agulha do exemplo com esse [código](https://github.com/VolosR/BoatGauges)

  TIME ID LEN DATA -

Idle

- 50ms 470  8  40* 01 00 00 00 00 00 1F
  - *C0 na primeira partida

Seta direita

- 50ms 470  8  42* 01 00 00 00 00 00 1F
  - *C0 na primeira partida - 42 250m 40

Seta esquerda

- 50ms 470  8  41* 01 00 00 00 00 00 1F
  - *C0 na primeira partida - 41 250ms 40

Pisca alerta

- 50ms 470  8  4B* 01 00 00 00 00 00 1F
  - *C0 na primeira partida - 48 250ms 4B

Posição do pedal

- 9ms 280  8  09 00 00 00 05 00* 05 05** - Desligado
  - *Pista 1 Pedal ou borboleta 00 até FA desligado
  - **Pista 2 Pedal ou borboleta 04-05 até CD desligado

Indicador de óleo - ta errado

- 500ms 58F  8  B8 F5 FF 7F 00 90 80 00achar idle* - Ligado
  - *23 71 - Luz ligada
  - C2 - Luz desligada

Rotação do motor

- 20ms 288  8  __ CB* 00 30 00 00 54 80** 08*** - Ligado
  - *Algum sensor
  - **Rotação 80 até um valor
  - ***Algum sensor

Iluminação painel

- 200ms 420  8  82 00 00 00 08 00* 00 04 - Ligado
  - *62 - Ligado
- 50ms 470  8   C0* 01 00** 00 00 00*** 00 1F - Carro Desligado
  - *40 - Depois da primeira partida
  - **64 - Ligado
  - ***06 - Ligado
- 50ms 728  7  04 01 01* 00** 02* 00 00 - Ligado
  - *00 e 06 - Chave off
  - **40 - Ligado

Tensão da bateria

- 200ms 420  5  87 20 A9* 00 00 - Ligado
  - *A9 - 13.45V, A8 - 13.4V

Temperatura da água

- 20ms 288  8  __ B7* 38 00 00 54* 85 0C - Ligado
  - LSB e MSB - Temperatura [5C B6]-88.5  [5C B5]-87.75 [5C B4]-87

Velocidade - possivelmente

- 10ms 4A0  8  00 00 00 00 00 00 00 00 - Ligado
