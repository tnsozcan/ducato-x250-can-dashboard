/*
  Fiat Ducato X250 bench - Arduino Nano ATmega328P
  Library: Cory J. Fowler MCP_CAN_lib / mcp_can 1.5.1
  https://github.com/coryjfowler/MCP_CAN_lib
  MCP2515 crystal: 8 MHz; CAN: 50 kbit/s; Serial: 115200.
  CS D10, INT D2, MOSI D11, MISO D12, SCK D13.
  C141 1362894080 / 503.001.210.203M: CAN-L pin 5, CAN-H pin 6.
  See docs/PINOUT.md; identify molded pin numbers, verify exact hardware.
  Do not identify CAN/power pins by wire color. Common GND required.

  NORMAL mod: gecerli alinan frame'lere MCP2515 otomatik ACK verir.
  Game-only public firmware. Boot sirasinda TX yok.
  TX0 register'lari non-blocking izlenir; hardware one-shot aktif.
  Basarili init, transceiver/kablo/cluster iletisiminin kaniti degildir.
*/
#include <SPI.h>
#include <mcp_can.h>

const uint8_t PIN_CS = 10;
const uint8_t PIN_INT = 2;
MCP_CAN canBus(PIN_CS);
uint32_t receivedFrames = 0;
uint32_t lastStatusMs = 0;

// Library register access is private. Use bounded SPI reads for verification
// and reception: mcp_can 1.5.1 readMsgBuf() can overrun its internal buffer
// if an invalid DLC > 8 is read (e.g. an SPI fault).
void readRegisters(uint8_t address, uint8_t *data, uint8_t count) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_CS, LOW);
  SPI.transfer(MCP_READ);
  SPI.transfer(address);
  for (uint8_t i = 0; i < count; ++i) data[i] = SPI.transfer(0);
  digitalWrite(PIN_CS, HIGH);
  SPI.endTransaction();
}

uint8_t readRegister(uint8_t address) {
  uint8_t value;
  readRegisters(address, &value, 1);
  return value;
}

void clearRxFlag(uint8_t mask) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_CS, LOW);
  SPI.transfer(MCP_BITMOD);
  SPI.transfer(MCP_CANINTF);
  SPI.transfer(mask);
  SPI.transfer(0);
  digitalWrite(PIN_CS, HIGH);
  SPI.endTransaction();
}

void printHex(uint32_t value, uint8_t digits) {
  for (int8_t shift = (digits - 1) * 4; shift >= 0; shift -= 4) {
    Serial.print((value >> shift) & 0x0FUL, HEX);
  }
}

void haltWithError(const __FlashStringHelper *message) {
  Serial.print(F("HATA: "));
  Serial.println(message);
  canBus.setMode(MODE_CONFIG);
  Serial.println(F("Durduruldu. Baglantilari kontrol edip Nano'yu resetleyin."));
  while (true) delay(1000);
}

bool configurationOK() {
  return readRegister(MCP_CNF1) == MCP_8MHz_50kBPS_CFG1 &&
         readRegister(MCP_CNF2) == MCP_8MHz_50kBPS_CFG2 &&
         readRegister(MCP_CNF3) == MCP_8MHz_50kBPS_CFG3 &&
         (readRegister(MCP_CANSTAT) & MODE_MASK) == MCP_NORMAL;
}

enum RunMode : uint8_t { IDLE, GAME_MODE };
RunMode runMode = IDLE;
// PC packet: game RPM speed_raw fuel_raw temp_raw light_b1 turn_b2 park (all DEC).
// Raw values are calibrated by the PC bridge; no TX starts without a packet.
uint16_t gameRpm = 0, gameSpeed = 0;
uint8_t gameFuel = 0, gameTemp = 40, gameLight = 0, gameTurn = 0, gamePark = 0;
uint32_t gameLastMs = 0, gameNext[7] = {0};
uint8_t gameCursor = 0;
bool gameStale = false, gameIgnition = true, gameCruise = false;
bool gameImmo = false;
uint32_t gameImmoStart = 0;
bool gameLampCheck = false;
uint32_t gameLampCheckStart = 0;
bool gameEngine = false, gameGlow = false;
uint16_t gameWarnings = 0;
uint16_t gameGlowDuration = 0;
uint32_t gameGlowStart = 0;
bool gameBatteryDelay = false;
uint32_t gameEngineStart = 0;
uint8_t testPayload[8] = {0};
uint16_t currentId = 0;
uint8_t currentDlc = 8;
bool txPending = false, singleSend = false, faultLatched = false;
uint32_t pendingStartMs = 0, txOK = 0, txFailed = 0, rxOverflows = 0;
char commandBuffer[96];
uint8_t commandLength = 0;
bool discardLine = false;
void bitModify(uint8_t address, uint8_t mask, uint8_t value) {
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_CS, LOW);
  SPI.transfer(MCP_BITMOD);
  SPI.transfer(address);
  SPI.transfer(mask);
  SPI.transfer(value);
  digitalWrite(PIN_CS, HIGH);
  SPI.endTransaction();
}
void stopSending() {
  runMode = IDLE;
  bitModify(MCP_TXB0CTRL, MCP_TXB_TXREQ_M, 0);
  txPending = false;
  singleSend = false;
}
void showStatus() {
  Serial.print(F("RX=")); Serial.print(receivedFrames);
  Serial.print(F(" TX_OK=")); Serial.print(txOK);
  Serial.print(F(" TX_FAIL=")); Serial.print(txFailed);
  Serial.print(F(" REC=")); Serial.print(readRegister(MCP_REC));
  Serial.print(F(" TEC=")); Serial.print(readRegister(MCP_TEC));
  Serial.print(F(" EFLG=0x")); printHex(readRegister(MCP_EFLG), 2);
  Serial.print(F(" LOCK=")); Serial.println(faultLatched ? 1 : 0);
}
void tripFault(const __FlashStringHelper *reason) {
  if (faultLatched) return;

  faultLatched = true;
  ++txFailed;
  stopSending();
  Serial.print(F("HATA: ")); Serial.println(reason);
  Serial.println(F("TX DURDU ve kilitlendi. Baglantiyi kontrol edip Nano'yu resetleyin."));
  showStatus();
}
bool checkHealth() {
  if (faultLatched) return false;
  const uint8_t e = readRegister(MCP_EFLG);
  if (e & MCP_EFLG_TXBO) { tripFault(F("BUS-OFF")); return false; }
  if (e & (MCP_EFLG_TXEP | MCP_EFLG_RXEP)) {
    tripFault(F("ERROR-PASSIVE")); return false;
  }
  if (readRegister(MCP_TEC) != 0 || (e & MCP_EFLG_TXWAR)) {
    tripFault(F("TX error counter > 0")); return false;
  }
  if (!configurationOK() || !(readRegister(MCP_CANCTRL) & MODE_ONESHOT)) {
    tripFault(F("NORMAL/hiz/one-shot ayari kayboldu")); return false;
  }
  return true;
}
void drainRX() {
  // Bounded reads; no per-frame Serial traffic. Drain both buffers promptly.
  for (uint8_t n = 0; n < 4; ++n) {
    const uint8_t flags = readRegister(MCP_CANINTF);
    if (flags == 0xFF) { tripFault(F("SPI: CANINTF=FF")); return; }
    if (!(flags & 3)) break;
    const uint8_t mask = flags & MCP_RX0IF ? MCP_RX0IF : MCP_RX1IF;
    uint8_t raw[13];
    readRegisters(mask == MCP_RX0IF ? MCP_RXB0SIDH : MCP_RXB1SIDH, raw, sizeof(raw));
    clearRxFlag(mask);
    if ((raw[4] & MCP_DLC_MASK) > 8) { tripFault(F("RX DLC > 8")); return; }
    ++receivedFrames;
  }
  const uint8_t overflow = readRegister(MCP_EFLG) & 0xC0;
  if (overflow) {
    ++rxOverflows; // Overflow events, not an exact lost-frame count.
    bitModify(MCP_EFLG, overflow, 0);
  }
}
void beginTransmit(bool reportSingle) {
  if (!checkHealth()) return;
  if (txPending || (readRegister(MCP_TXB0CTRL) & MCP_TXB_TXREQ_M)) {
    tripFault(F("TX0 hala mesgul")); return;
  }
  // Use TX0 directly: mcp_can 1.5.1 sendMsgBuf has a 2500us timeout,
  // shorter than a worst-case 8-byte frame at 50 kbit/s. Do not edit library.
  uint8_t raw[13] = {0};
  raw[0] = currentId >> 3;
  raw[1] = (currentId & 7) << 5; // EXIDE=0, standard data frame
  raw[4] = currentDlc;
  memcpy(raw + 5, testPayload, 8);
  bitModify(MCP_CANINTF, MCP_TX0IF, 0);
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(PIN_CS, LOW);
  SPI.transfer(MCP_WRITE);
  SPI.transfer(MCP_TXB0CTRL + 1); // TXB0SIDH
  for (uint8_t i = 0; i < sizeof(raw); ++i) SPI.transfer(raw[i]);
  digitalWrite(PIN_CS, HIGH);
  SPI.endTransaction();
  bitModify(MCP_TXB0CTRL, MCP_TXB_TXREQ_M, MCP_TXB_TXREQ_M);
  txPending = true;
  singleSend = reportSingle;
  pendingStartMs = millis();
}
void pollTransmit() {
  if (!txPending) return;
  const uint8_t ctrl = readRegister(MCP_TXB0CTRL);
  if (ctrl & (MCP_TXB_TXERR_M | MCP_TXB_ABTF_M | MCP_TXB_MLOA_M)) {
    tripFault(F("TX hata/abort/arbitration loss (one-shot)")); return;
  }
  if (!(ctrl & MCP_TXB_TXREQ_M)) {
    if (!(readRegister(MCP_CANINTF) & MCP_TX0IF)) {
      tripFault(F("TX tamamlanma onayi yok")); return;
    }
    txPending = false;
    bitModify(MCP_CANINTF, MCP_TX0IF, 0);
    ++txOK;
    if (singleSend) Serial.println(F("TX OK (CAN seviyesinde; ibre tepkisi garanti degil)."));
    singleSend = false;
  } else if (millis() - pendingStartMs >= 20UL) {
    tripFault(F("TX timeout 20ms"));
  }
}
bool parseNumber(const char *s, uint8_t base, uint16_t maximum, uint16_t &value) {
  if (!s || !*s) return false;
  if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
  if (!*s) return false;
  uint16_t result = 0;
  while (*s) {
    uint8_t d;
    if (*s >= '0' && *s <= '9') d = *s - '0';
    else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
    else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
    else return false;
    if (d >= base || d > maximum || result > (maximum - d) / base) return false;
    result = result * base + d;
    ++s;
  }
  value = result;
  return true;
}
void processCommand(char *line) {
  char *tokens[12];
  uint8_t count = 0;
  char *p = strtok(line, " \t");
  while (p) {
    if (count >= 12) { Serial.println(F("HATA: Fazla arguman.")); return; }
    tokens[count++] = p;
    p = strtok(NULL, " \t");
  }
  if (!count) return;
  const char *cmd = tokens[0];
  if (!strcmp(cmd, "game") && (count == 7 || count == 8 || count == 10 || count == 12)) {
    uint16_t values[11] = {0};
    values[7] = 1; // Legacy packets keep illumination on.
    const uint16_t limits[11] = {6000, 2800, 135, 156, 255, 255, 1, 1, 1, 1, 16383};
    for (uint8_t i = 0; i < count - 1; ++i) {
      if (!parseNumber(tokens[i + 1], 10, limits[i], values[i])) {
        Serial.println(F("HATA: gecersiz game paketi.")); return;
      }
    }
    if (values[3] < 40 || (values[4] & ~0x3CU) || (values[5] & ~0x60U)) {
      Serial.println(F("HATA: game sicaklik/aydinlatma alani.")); return;
    }
    if (!checkHealth()) return;
    const bool enteringGame = runMode != GAME_MODE;
    if (enteringGame) {
      stopSending();
      memset(gameNext, 0, sizeof(gameNext));
      gameCursor = 0;
      Serial.println(F("GAME MODE ACTIVE"));
    }
    const bool ignition = values[7] != 0;
    const bool engine = count == 12 ? values[9] != 0 : values[0] > 300;
    if (ignition && ((!enteringGame && !gameIgnition) || (enteringGame && !engine))) {
      gameImmo = true; gameImmoStart = millis();
      gameLampCheck = true; gameLampCheckStart = millis();
      gameGlow = true; gameGlowStart = millis();
      // Approximation, not the actual Ducato ECU preheat calculation.
      gameGlowDuration = values[3] >= 100 ? 1000 : (values[3] >= 60 ? 3000 : 5000);
    }
    if (!ignition) { gameImmo = false; gameGlow = false; gameLampCheck = false; }
    if (engine) gameGlow = false;
    if (!enteringGame && !gameEngine && engine && ignition) {
      gameBatteryDelay = true; gameEngineStart = millis();
    }
    if (enteringGame || !ignition || !engine) gameBatteryDelay = false;
    gameEngine = engine; gameWarnings = count == 12 ? values[10] : 0;
    gameIgnition = ignition; gameCruise = values[8] != 0;
    gameRpm = ignition ? values[0] : 0;
    gameSpeed = ignition ? values[1] : 0;
    // Ignition off: command empty fuel and cold/minimum temperature.
    gameFuel = ignition ? values[2] : 0;
    gameTemp = ignition ? values[3] : 40;
    gameLight = values[4]; gameTurn = values[5]; gamePark = values[6];
    gameLastMs = millis(); gameStale = false; runMode = GAME_MODE;
    return;
  }
  if ((!strcmp(cmd, "stop") || !strcmp(cmd, "allstop")) && count == 1) {
    stopSending(); Serial.println(F("TX STOP")); return;
  }
  if (!strcmp(cmd, "status") && count == 1) { showStatus(); return; }
  Serial.println(F("HATA: game-only firmware; commands: game, stop, status."));
}
void readCommands() {
  // Bounded work per loop so continuous serial input cannot starve CAN.
  for (uint8_t n = 0; n < 24 && Serial.available(); ++n) {
    const char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (!discardLine && commandLength) {
        commandBuffer[commandLength] = '\0';
        processCommand(commandBuffer);
      }
      commandLength = 0;
      discardLine = false;
    } else if (!discardLine) {
      if (commandLength < sizeof(commandBuffer) - 1) commandBuffer[commandLength++] = c;
      else {
        // A malformed/truncated stop must not leave a repeat running.
        stopSending();
        discardLine = true;
        Serial.println(F("HATA: Satir cok uzun; TX durdu, satir atlandi."));
      }
    }
  }
}
void setup() {
  Serial.begin(115200);
  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH);
  pinMode(PIN_INT, INPUT_PULLUP);
  SPI.begin();
  delay(500);
  Serial.println(F("Ducato X250 GAME ONLY | MCP2515 8 MHz | 50 kbit/s | CS D10 INT D2"));
  const uint8_t result = canBus.begin(MCP_ANY, CAN_50KBPS, MCP_8MHZ);
  if (result != CAN_OK) {
    Serial.print(F("begin() sonuc=0x")); printHex(result, 2); Serial.println();
    haltWithError(F("MCP2515 initialize basarisiz."));
  }
  if (canBus.setMode(MCP_NORMAL) != CAN_OK) haltWithError(F("NORMAL modu secilemedi."));
  Serial.print(F("CNF1/2/3=0x")); printHex(readRegister(MCP_CNF1), 2);
  Serial.print(F("/0x")); printHex(readRegister(MCP_CNF2), 2);
  Serial.print(F("/0x")); printHex(readRegister(MCP_CNF3), 2);
  Serial.print(F(" CANSTAT=0x")); printHex(readRegister(MCP_CANSTAT), 2); Serial.println();
  if (!configurationOK()) haltWithError(F("Hiz register'lari veya CANSTAT dogrulanamadi."));
  if (canBus.enOneShotTX() != CAN_OK) haltWithError(F("One-shot acilamadi."));
  // CNF=84/E5/83: BRP=4, 16 TQ; 8MHz/(2*(4+1)*16)=50000.
  Serial.println(F("BASARILI: NORMAL, otomatik ACK aktif, one-shot TX aktif."));
  Serial.println(F("Ready: start the PC game bridge. No boot TX. Commands: game / stop / status."));
  lastStatusMs = millis();
}
void runGame(uint32_t now) {
  const uint32_t age = now - gameLastMs;
  if (age > 1000UL && !gameStale) {
    gameStale = true;
    gameRpm = gameSpeed = 0;
    gameLight = gameTurn = 0;
    Serial.println(F("ETS2 VERI KESILDI: hiz/devir sifirlaniyor."));
  }
  if (age > 3000UL) {
    stopSending();
    Serial.println(F("ETS2 VERI YOK: TX durdu."));
    return;
  }
  if (gameGlow && now - gameGlowStart >= gameGlowDuration) gameGlow = false;
  if (gameBatteryDelay && now - gameEngineStart >= 700UL) gameBatteryDelay = false;
  if (gameLampCheck && now - gameLampCheckStart >= 4000UL) gameLampCheck = false;
  if (txPending) return;
  const uint16_t ids[7] = {0x281, 0x2A0, 0x380, 0x180, 0x286, 0x39A, 0x3C0};
  const uint16_t periods[7] = {50, 100, 250, 50, 100, 250, 250};
  for (uint8_t n = 0; n < 7; ++n) {
    const uint8_t slot = (gameCursor + n) % 7;
    if (int32_t(now - gameNext[slot]) < 0) continue;
    memset(testPayload, 0, 8);
    currentId = ids[slot]; currentDlc = 8;
    if (slot == 0) {
      const uint16_t raw = gameRpm * 8U;
      testPayload[2] = 0x80; testPayload[3] = gameTemp; testPayload[4] = 1;
      if (gameIgnition && !gameStale) {
        if (gameCruise) testPayload[1] |= 0x02;
        if (gameGlow) testPayload[1] |= 0x10;
        if (!gameEngine || (gameWarnings & 1)) testPayload[1] |= 0x80;
        if (gameWarnings & 64) testPayload[1] |= 0x40;
        if (gameWarnings & 256) testPayload[1] |= 0x20;
        if (gameWarnings & 1024) testPayload[1] |= 0x08;
        if (gameWarnings & 128) testPayload[7] |= 0x80;
        if (gameEngine && (gameWarnings & 2048)) testPayload[7] |= 0x01; // Bench verified blinking oil.
      }
      testPayload[5] = raw & 255; testPayload[6] = raw >> 8;
    } else if (slot == 1) {
      currentDlc = 4;
      testPayload[0] = gameSpeed >> 8; testPayload[1] = gameSpeed & 255;
      testPayload[3] = 0xC3;
    } else if (slot == 2) {
      const uint8_t body[8] = {0x08, 0, 0x40, 0x59, 0, 0, 0x0B, 4};
      memcpy(testPayload, body, 8); testPayload[5] = gameFuel;
      if (!gameIgnition) testPayload[0] &= ~0x08;
      if (gamePark) testPayload[0] |= 0x20;
      if (gameIgnition && !gameStale) {
        if (gameWarnings & 8) testPayload[1] = 0x0C;
        if (gameWarnings & 16) testPayload[4] |= 0x02;
        if (gameWarnings & 512) testPayload[0] |= 0x80;
      }
      if (gameImmo) {
        const uint32_t elapsed = now - gameImmoStart;
        if (elapsed >= 3000UL) gameImmo = false;
        else if ((elapsed / 500UL) % 2 == 0) testPayload[6] = 0x0A;
      }
    } else if (slot == 3) {
      currentDlc = 6;
      testPayload[1] = gameLight; testPayload[2] = gameTurn;
    } else if (slot == 4) {
      const uint8_t absStatus[8] = {0, 0, 0, 0, 0x08, 0xDB, 0x08, 0x7F};
      memcpy(testPayload, absStatus, 8);
      if (gameIgnition && !gameStale && ((gameLampCheck && now - gameLampCheckStart < 3000UL) || (gameWarnings & 4)))
        testPayload[1] = 0x20; // Verified ABS ON, brake OFF.
    } else if (slot == 5) {
      // 39A zero status: bench observed airbag/belt lamps off.
      if (gameIgnition && !gameStale && (gameWarnings & 32)) testPayload[2] = 1;
      // Airbag lamp check, then return to verified all-zero status.
      if (gameIgnition && !gameStale && gameLampCheck && now - gameLampCheckStart < 4000UL)
        testPayload[0] = 0x80; // Verified steady airbag lamp.
      if (gameIgnition && !gameStale) {
        if (gameWarnings & 4096) testPayload[0] = 0x80;
        if (gameWarnings & 8192) testPayload[0] = 0x40; // Bench verified fast airbag blink.
      }
    } else {
      // Reproduce the verified DLC8 battery-lamp bench command.
      if (gameIgnition && !gameStale && (!gameEngine || gameBatteryDelay || (gameWarnings & 2)))
        testPayload[0] = 0x40;
    }
    gameNext[slot] = now + periods[slot];
    gameCursor = (slot + 1) % 7;
    beginTransmit(false);
    return;
  }
}
void loop() {
  drainRX(); pollTransmit(); readCommands();
  const uint32_t now = millis();
  if (now - lastStatusMs >= 100UL) { lastStatusMs = now; checkHealth(); }
  if (!faultLatched && runMode == GAME_MODE) runGame(now);
}
