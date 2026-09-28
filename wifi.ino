void iniciarPortal()
{
  setStatusRGB(0, 0, 255);

  WiFi.mode(WIFI_AP);
  WiFi.softAP("M5NANOC6_CONFIG");

  Serial.println("Portal Ativo!");
  Serial.println("WiFi: M5NANOC6_CONFIG");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPage);
  });

  server.on("/salvar", HTTP_POST, salvarWifi);

  server.begin();
}

bool conectarWifi() 
{
  setStatusRGB(255, 180, 0);  
  
  prefs.begin("wifi", true);
  String ssid = prefs.getString("ssid", "");
  String password = prefs.getString("senha", "");
  prefs.end();

  if (ssid == "") 
  {
    setStatusRGB(255, 0, 0);
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  
  Serial.println("Conectando a:");
  Serial.println(ssid);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) 
  {
    setStatusRGB(255, 180, 0);
    delay(250);
    setStatusRGB(0, 0, 0);
    delay(250);    
    tentativas++;
  }
  digitalWrite(LED, LOW);
  return WiFi.status() == WL_CONNECTED;
}

void salvarWifi() 
{
  Serial.println("Salvando rede...");
  String novoSSID = server.arg("ssid");
  String novaSenha = server.arg("senha");

  prefs.begin("wifi", false);
  prefs.putString("ssid", novoSSID);
  prefs.putString("senha", novaSenha);
  prefs.end();

  server.send(200, "text/html", "<h2>Configuracao salva! Reiniciando...</h2>");
  delay(2000);
  ESP.restart();
}

void zerarConfiguracoes() 
{
  Serial.println("Limpando Memoria...");

  prefs.begin("wifi", false);
  prefs.clear(); 
  prefs.end();
  
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.print("WiFi zerado. Reiniciando...\n");
  udp.endPacket();

  Serial.println("WiFi zerado. Reiniciando...\n");
  
  for(int i=0; i<10; i++) {
    digitalWrite(LED, HIGH); delay(100);
    digitalWrite(LED, LOW); delay(100);
  }
  ESP.restart(); 
}