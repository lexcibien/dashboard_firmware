# Configurações legadas de placas

Os arquivos deste diretório vieram da configuração antiga do PlatformIO para o
RP2350/RP2040 e para o ESP32-C3. Eles são mantidos como referência histórica e
não são consumidos pelo build atual do ESP-IDF (`idf.py`).

Em particular:

- `json/` contém descrições de placas do PlatformIO;
- `pinouts/User_Setup.h` contém a configuração antiga do TFT_eSPI;
- `rpipico2/` e `super_mini_esp32c3/` contêm índices de placas do PlatformIO.

A pinagem usada pelo smoke test atual está documentada em `README.md` e em
`sdkconfig.defaults`, mas continua provisória até ser verificada no hardware.
