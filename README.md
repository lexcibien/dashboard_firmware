# Dashboard firmware

## ESP-IDF + TFT_eSPI

O build ativo usa ESP-IDF e CMake para o ESP32-C3. O aplicativo fica em
`main/`, que é o nome convencional do componente principal no ESP-IDF.

### Stack fixada

- ESP-IDF v6.1.
- Arduino-ESP32 4.0.0-RC1 como componente ESP-IDF, usado somente como ponte
  para o TFT_eSPI nesta etapa.
- TFT_eSPI como submódulo em `components/TFT_eSPI`.
- Alvo `esp32c3`.

O Arduino 4.0.0-RC1 é pré-lançamento. Se a combinação não compilar, o fallback
documentado é ESP-IDF 5.5.5 com Arduino-ESP32 3.3.12.

### Dependências do projeto

Após ativar o ESP-IDF:

```bash
git submodule update --init --recursive
```

O Arduino deve estar em `components/arduino` e o TFT_eSPI em
`components/TFT_eSPI`. O commit do TFT_eSPI deve permanecer fixado no
submódulo, evitando que uma atualização inesperada altere o build.

### Configuração e comandos ESP-IDF

```bash
idf.py set-target esp32c3
idf.py reconfigure
idf.py menuconfig
idf.py build
idf.py size
```

No `menuconfig`, configure o driver GC9A01, 240x240, SPI, os pinos provisórios
do smoke test e desative o autostart de `setup()`/`loop()` do Arduino. O
`app_main()` chama `initArduino()` diretamente. O Arduino-ESP32 e o TFT_eSPI
são uma ponte temporária de migração; a arquitetura final deverá usar as APIs
nativas do ESP-IDF quando o teste de hardware estiver validado.

Pinagem provisória do primeiro teste TFT:

| Sinal | GPIO |
|-------|------|
| MOSI  | 19   |
| SCLK  | 18   |
| CS    | 20   |
| DC    | 17   |
| RST   | 16   |

Essa pinagem ainda não é a pinagem de produção: GPIO18/19 podem conflitar com
USB nativo e GPIO20 aparece como RX na variante local. A UART com o RP2040 só
deve ser implementada depois de uma nova revisão da fiação.

Para gravar e monitorar:

```bash
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
idf.py -p /dev/ttyACM0 flash monitor
```

Encerre o monitor com `Ctrl+]`. Se a configuração de componentes mudar,
recrie o build:

```bash
idf.py fullclean
idf.py reconfigure
idf.py build
```

O demo aceita somente comandos de desenvolvimento:

```text
VALUE=0..4095
MENU=0..5
```

Os comandos de texto não são o protocolo de produção.

## Referências legadas do RP2350/PlatformIO

Os arquivos em `boards/`, incluindo `boards/pinouts/User_Setup.h`, são
referências da configuração antiga do PlatformIO. Eles não são consumidos pelo
build atual do ESP-IDF, não definem a pinagem de produção do ESP32-C3 e devem
ser tratados como legado até serem substituídos por uma configuração de placa
específica do ESP-IDF. A descrição desses arquivos está em
[`boards/README.md`](boards/README.md).

O OLED SSD1306 e o shifter físico pertencem ao firmware separado do RP2040.
Este repositório é responsável pelo dashboard no ESP32-C3; não mova essas
responsabilidades para cá.

### Display legado do RP2350

Segui esses vídeos:

- [Biblioteca](https://www.manualdomaker.com/article/display-lcd-gc-9-a01-redondo/)
- [Conexão de pinos](https://www.youtube.com/watch?v=2wPPcgb-Vvg)

Esta configuração documenta somente o experimento antigo do RP2350. O arquivo
`.pio/libdeps/<env>/TFT_eSPI/User_Setup_Select.h` não faz parte do build atual e
não deve ser editado para configurar o projeto ESP-IDF.

Na configuração antiga, eram adicionadas estas linhas acima de
`#ifndef USER_SETUP_LOADED`:

```cpp
#ifdef TFT_ESPI_USER_SETUP_PATH  // Use external User_Setup.h if defined

  #define USER_SETUP_LOADED

  // Define TFT_ESPI_USER_SETUP_PATH as a string literal
  // Example (PlatformIO):
  // -D TFT_ESPI_USER_SETUP_PATH=\"../configs/your_board/User_Setup.h\"
  #include TFT_ESPI_USER_SETUP_PATH

#endif
```

### Conexão rpipico2

| RP2350 | GC9A01 |
|--------|--------|
| 3V3    | VCC    |
| GND    | GND    |
| GP18   | SCL    |
| GP19   | SDA    |
| GP17   | DC     |
| GP20   | CS     |
| GP16   | RST    |
