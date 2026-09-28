void executa_comando(String cmd) {
  Serial.println("> " + cmd);

  if (cmd == "RESET_WIFI") {
    zerarConfiguracoes(); 
  }
  else if (cmd == "LED_ON") {
    blinkAtivo = false;
    estadoLed = true;
    digitalWrite(LED, HIGH);
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.print("LED ligado\n");
    udp.endPacket();
  }
  else if (cmd == "LED_OFF") {
    blinkAtivo = false;
    estadoLed = false;
    digitalWrite(LED, LOW);
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.print("LED desligado\n");
    udp.endPacket();
  }
  else if (cmd == "TEMP") {
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.printf("CPU Temp: %.2f\n", temperatureRead());
    udp.endPacket();
  }
  else if (cmd == "CPU") // Informações sobre a CPU
  {
    //Serial.println(udp.remoteIP());
    //Serial.println(udp.remotePort());
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.printf("Modelo: %s\n", ESP.getChipModel());
    udp.printf("Revisao: %d\n", ESP.getChipRevision());
    udp.printf("Nucleos: %d\n", ESP.getChipCores());
    udp.printf("CPU: %d MHz\n", ESP.getCpuFreqMHz());
    udp.printf("RAM livre: %u bytes\n", ESP.getFreeHeap());
    udp.endPacket();
  }
  else if (cmd == "RAM") // Informações sobre a RAM
  {
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.printf("Heap livre: %u\n", ESP.getFreeHeap());
    udp.printf("Menor heap livre: %u\n", ESP.getMinFreeHeap());
    udp.printf("Maior bloco livre: %u\n", ESP.getMaxAllocHeap());
    udp.endPacket();
  }
  else if (cmd == "FLASH") // Informações sobre a flash
  {
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.printf("Flash total: %u\n", ESP.getFlashChipSize());
    udp.printf("Velocidade Flash: %u\n", ESP.getFlashChipSpeed());
    udp.printf("Tamanho Sketch: %u\n", ESP.getSketchSize());
    udp.printf("Espaco livre: %u\n", ESP.getFreeSketchSpace());
    udp.endPacket();
  }
  else if (cmd == "INIT") // Motivo do reset
  {
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.printf("Motivo reset: %d\n", esp_reset_reason());    
    udp.endPacket();
  }
  else if (cmd == "UPTIME") // Tempo ligado
  {
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.printf("Uptime: %lu ms\n", millis());    
    udp.endPacket();
  }
  else if (cmd == "MAC") // MAC address
  {
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.printf("MAC: ");
    udp.println(WiFi.macAddress());
    udp.endPacket();
  }
  else if (cmd == "NET_INFO") // MAC address
  {
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.printf("IP: ");
    udp.println(WiFi.localIP());
    udp.printf("Gateway: ");
    udp.println(WiFi.gatewayIP());
    udp.printf("Mascara de rede: ");
    udp.println(WiFi.subnetMask());
    udp.printf("RSSI: %d dbm\n", WiFi.RSSI());
    udp.printf("Nome da Rede: %s\n", WiFi.SSID());
    udp.endPacket();
    }  
  else if (cmd.startsWith("LED_PISCA")) // Comando para piscar o LED uma quantidade de vezes
  {
    int piscadas = 10;
    int tempo = 250;

    int p1 = cmd.indexOf(':');
    int p2 = cmd.indexOf(':', p1 + 1);

    if (p1 > 0 && p2 > 0) 
    {
      piscadas = cmd.substring(p1 + 1, p2).toInt();
      tempo = cmd.substring(p2 + 1).toInt();
    }

    for (int i = 0; i < piscadas; i++) 
    {
      digitalWrite(LED, HIGH);  delay(tempo);
      digitalWrite(LED, LOW);   delay(tempo);
    }

    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.printf("LED piscou %d vezes com %d ms\n", piscadas, tempo);
    udp.endPacket();
  }
  else if (cmd.startsWith("LED_BLINK")) // Comando para piscar led com tempo
  {
    int p = cmd.indexOf(':');

    if (p > 0) 
    {
      intervaloBlink = cmd.substring(p + 1).toInt();
    }

    blinkAtivo = true;

    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.printf("Blink iniciado (%lu ms)\n", intervaloBlink);
    udp.endPacket();
  }
  else if (cmd == "TIME") // MAC address
  {
    GetDataHora();
  }
  else 
  {
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.print("Comando Invalido\n");
    udp.endPacket();
  }
}
