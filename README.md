# Dashboard firmware

Firmware do dashboard no ESP32-C3 Super Mini, com display GC9A01 redondo de
240x240. O caminho ativo agora é nativo do ESP-IDF: `esp_lcd` transporta os
pixels por SPI e `esp_lvgl_port` integra o display ao LVGL 9.

## Estado do exemplo

O exemplo atual contém:

- inicialização nativa do barramento SPI e do driver `esp_lcd_gc9a01`;
- buffer parcial DMA de 240x20 pixels, com double buffering;
- UI LVGL com `lv_arc`, `lv_line`, labels e seis botões de seleção;
- valor de gauge simulado por um timer de 33 ms, apenas para validar a
  renderização e os limites da agulha.

UART, telemetria do RP2040, NVS, Wi-Fi e servidor web continuam fora deste
exemplo. O valor simulado não é um protocolo de produção.

Os headers de bitmap/font antigos permanecem no repositório como material
histórico, mas não são dependências do build ativo.
O OLED SSD1306 e o shifter físico continuam sendo responsabilidade do firmware
separado do RP2040.

## Versões e dependências

- ESP-IDF 6.1 (a configuração existente do workspace aponta para essa versão);
- `lvgl/lvgl` 9.5.0;
- `espressif/esp_lvgl_port` 2.9.0;
- `espressif/esp_lcd_gc9a01` 2.0.4;
- alvo `esp32c3`.

As versões estão fixadas nos manifests de `components/display` e
`components/dashboard`. O `dependencies.lock` deve ser regenerado pelo
ESP-IDF Component Manager, não editado à mão.

## Build

Ative uma instalação completa do ESP-IDF antes dos comandos. O `idf.py` não
estava disponível no `PATH` durante a preparação deste patch, portanto o build
não foi validado nesta sessão.

```bash
source "$IDF_PATH/export.sh"
idf.py set-target esp32c3
idf.py reconfigure
idf.py build
idf.py size
```

Se o Component Manager solicitar a atualização de dependências, execute
`idf.py reconfigure` novamente. Para gravar e monitorar:

```bash
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
idf.py -p /dev/ttyACM0 flash monitor
```

## Pinagem provisória

Os defaults abaixo reaproveitam a pinagem do smoke test anterior, mas ainda
não são uma pinagem de produção verificada. Altere-os em `idf.py menuconfig`,
no menu `Dashboard display`, depois de conferir o esquema da sua placa e do
módulo GC9A01. GPIO18/19 podem conflitar com USB nativo, e GPIO20 pode ser
usado como RX dependendo da variante.

| Sinal | Default |
| --- | ---: |
| MOSI | GPIO6 |
| SCLK | GPIO4 |
| CS | GPIO7 |
| DC | GPIO10 |
| RST | GPIO3 |
| Backlight | externo/sempre ligado |

O clock inicia em 20 MHz. Só aumente depois de validar cor, rotação, ausência
de tearing e estabilidade do barramento no hardware real. A opção
`DASHBOARD_LCD_SWAP_BYTES` fica habilitada porque o painel SPI normalmente
recebe RGB565 em big-endian; se as cores aparecerem trocadas, confirme essa
opção com um teste de barras de cor.

## Assets no fluxo do LVGL

Não é necessário transformar os seis gauges em arrays C manualmente para este
exemplo. A tela usa widgets vetoriais do LVGL, reduzindo o firmware e deixando
agulha, faixa e labels atualizáveis sem copiar um framebuffer inteiro.

Quando a arte original for necessária, prefira este fluxo:

1. Use o [LVGL Image Converter](https://lvgl.io/tools/imageconverter) para
   converter JPG/PNG/BMP diretamente para um `.c` LVGL 9 ou para um `.bin`.
   O [código do conversor](https://github.com/lvgl/lv_img_conv) também pode ser
   executado localmente.
2. Para a conversão no build, `esp_lvgl_port` fornece
   `lvgl_port_create_c_image(input, output, color_format, compression)` e
   `lvgl_port_add_images(...)`. Prefira `RGB565` para o GC9A01 e experimente
   `RLE` ou `LZ4` quando a imagem tolerar a decodificação.
3. O conversor offline `LVGLImage.py` do LVGL 9 é apropriado para PNG. Os JPG
   atuais em `assets/` podem ser pré-processados por uma ferramenta de imagem
   automatizável, como ImageMagick, e então passados ao conversor LVGL; os
   arquivos gerados devem continuar sendo artefatos de build, não fontes
   editadas à mão.
4. Para fontes, use `lv_font_conv` ou o gerador de fontes do LVGL Pro, em vez
   de reutilizar diretamente um formato proprietário de outra biblioteca. Para um fluxo
   visual de telas, o LVGL Pro/Editor exporta C LVGL; para um projeto aberto e
   simples, `lvgl_port_create_c_image` e os widgets nativos são suficientes.

Exemplo de integração no `CMakeLists.txt` de um componente que já depende de
`esp_lvgl_port`:

```cmake
lvgl_port_create_c_image("images/gauge.png" "${CMAKE_CURRENT_BINARY_DIR}/lvgl_images" "RGB565" "RLE")
lvgl_port_add_images(${COMPONENT_LIB} "${CMAKE_CURRENT_BINARY_DIR}/lvgl_images")
```

No código LVGL 9, declare a imagem com `LV_IMAGE_DECLARE(gauge)` e use-a com
`lv_image_set_src`. O nome deve corresponder ao arquivo gerado. O conversor
oficial preserva o formato esperado pelo LVGL e evita o acoplamento ao
APIs específicas de bibliotecas antigas.

## Arquitetura planejada

O próximo passo é conectar a UI a um estado tipado vindo do caminho
`PC -> USB CDC do RP2040 -> UART -> ESP32-C3`. O protocolo deverá ser
versionado, enquadrado e validado com CRC antes de substituir o timer de demo.
O dashboard não deve depender de Wi-Fi nem impedir o funcionamento do shifter
quando o link estiver ausente.

## Referências legadas do RP2350/PlatformIO

Os arquivos restantes em `boards/` são referências da configuração antiga do
PlatformIO. Não são consumidos pelo build ESP-IDF atual. A descrição está em
[boards/README.md](boards/README.md).

### Display legado do RP2350

Seguem as referências do experimento antigo:

- [Biblioteca](https://www.manualdomaker.com/article/display-lcd-gc-9-a01-redondo/)
- [Conexão de pinos](https://www.youtube.com/watch?v=2wPPcgb-Vvg)

### Conexão rpipico2

| RP2350 | GC9A01 |
| --- | --- |
| 3V3 | VCC |
| GND | GND |
| GP18 | SCL |
| GP19 | SDA |
| GP17 | DC |
| GP20 | CS |
| GP16 | RST |
