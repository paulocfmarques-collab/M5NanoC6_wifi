void configureNTP()
{  
  configTime(
    gmtOffset_sec,
    daylightOffset_sec,
    ntpServer
  );

  Serial.println("Obtendo hora...");

  struct tm timeinfo;

  while (!getLocalTime(&timeinfo)) {
    Serial.println("Falha ao obter hora");
    delay(1000);
  }

  Serial.println(&timeinfo, "%d/%m/%Y %H:%M:%S");
  setStatusRGB(128, 0, 255);
}

void rainbowBoot()
{
  setStatusRGB(255, 0, 0);
  delay(200);

  setStatusRGB(0, 255, 0);
  delay(200);

  setStatusRGB(0, 0, 255);
  delay(200);

  setStatusRGB(0, 0, 0);
}

void setStatusRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t brilho)
{
  led.setBrightness(brilho);
  led.setPixelColor(0, led.Color(r, g, b));
  led.show();
}

void GetDataHora()
{
  struct tm timeinfo;
  
  // Tenta obter o horário local configurado na pilha lwIP
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Falha ao obter estrutura de tempo local");
    return;
  }

  // Criamos buffers de texto estáticos para evitar lixo de memória (Garbage bytes)
  char bufferData[12]; // "DD/MM/AAAA\0"
  char bufferHora[9];  // "HH:MM:SS\0"

  // Formata os dados de maneira segura nas variáveis de texto
  sprintf(bufferData, "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  sprintf(bufferHora, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  String dataHoraCompleta = String(bufferData) + " - " + String(bufferHora);
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.println(dataHoraCompleta);
  udp.endPacket();
  Serial.println(dataHoraCompleta);
}