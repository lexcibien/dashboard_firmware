# Dashboard firmware

## Display - Apenas RP2350

Segui esses vídeos:

- [Biblioteca](https://www.manualdomaker.com/article/display-lcd-gc-9-a01-redondo/)
- [Conexão de pinos](https://www.youtube.com/watch?v=2wPPcgb-Vvg)

Dentro do arquivo `.pio/libdeps/<env>/TFT_eSPI/User_Setup_Select.h`, adicionar essas linhas acima de `#ifndef USER_SETUP_LOADED`:

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
