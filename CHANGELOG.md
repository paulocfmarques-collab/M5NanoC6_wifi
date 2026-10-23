# Changelog

## 2026-10-21

- LCD com duas telas e ícones HD44780: início com data/hora, máxima/mínima,
  temperatura atual, umidade e descrição do clima; rede com estado, IP, SSID,
  RSSI e canal. Descrições/SSID longos usam rolagem.
- Botão GPIO9: toque alterna páginas, 3 s pressionado desliga; desligado, toque
  religa no início sem alternar. Rede volta ao início após 15 s sem interação.
- Prévia web com símbolos dos ícones e API `lcd.tela`; testes nativos do driver
  e botão aprovados. Compilação e validação física continuam com o usuário.


- `HA_STATUS` por UDP/Serial mostra conexão e andamento; `HA_DISCOVERY` agenda
  republicação das 12 entidades e solicita reconexão quando necessário.
- Barra OTA visível desde a abertura de `/ota`, percentual numérico e etapas
  de envio/validação/reinício; erro encerra a indicação de espera e permite repetir.
- Comandos `HA_USER` e `HA_PASSWORD` por UDP/Serial retornam as credenciais
  MQTT salvas, incluindo a senha em texto, conforme solicitado. HELP e docs
  atualizados; sem alteração das prefs ou da API web.
- Integração Home Assistant por MQTT Discovery, com 12 entidades, identidade
  baseada na MAC, disponibilidade por Last Will e redescoberta no nascimento do HA.
- Página `/homeassistant` e API de configuração persistida no namespace `m5ha`,
  broker/porta/usuário/senha/intervalo; senha não é devolvida pela API.
- Controles de LCD/LED, botões NTP/IR e sensores de CPU, heap, uptime, RSSI e
  clima de Porto Alegre. Estado atualizado também após ações de outros canais.
- Métricas MQTT reais no Prometheus; reconexão com intervalo de 30 segundos.
- Dependência PubSubClient 2.8, já disponível neste ambiente. Compilação pelo usuário.
- Testes locais aprovados: serviço real com ArduinoJson real e broker/hardware
  simulados, discovery das 12 entidades, persistência, comandos/reconexão,
  formulário de configuração e contadores MQTT no Prometheus. Broker/HA reais
  e tamanho final do firmware dependem de compilação e gravação pelo usuário.
- Padronizada a resposta de `help` com o cabeçalho `COMANDOS` e um comando por
  linha; atualizados o guia local e o catálogo geral para facilitar o parser.


## 2026-10-19

- Corrigida autenticação involuntária do ArduinoOTA no core ESP32 3.3.12:
  removida a chamada `setPassword("")`, que gerava hash da senha vazia.
  Instale a correção por OTA web ou USB antes de voltar ao envio pela IDE.
- Métricas UDP: `esp_udp_listener_up`, `esp_udp_packets_received_total`,
  `esp_udp_replies_sent_total` e `esp_udp_send_errors_total`.
- Abertura da porta verifica o retorno real; respostas verificam abertura,
  quantidade escrita e resultado do envio. Contadores em RAM por inicialização.
- Socket fechado na recuperação de Wi-Fi e no portal AP; descarte do restante
  de datagramas longos evita bloquear a recepção dos próximos.
- Documentação das séries, critérios de contagem e consultas PromQL.
- Teste nativo aprovado: valores iniciais, estado do listener, incrementos de
  recepção/sucesso/erro, preservação ao reabrir, reset no boot e formato Prometheus.
  Compilação do firmware e teste físico continuam a cargo do usuário.

## 2026-10-31 — Reset dos contadores NVS

- Adicionado `reset_counters` para zerar os contadores persistentes de
  inicialização e watchdog sem apagar configurações Prometheus.
- Atualizados o menu `help`, o guia de comandos e o catálogo consolidado.

## 2026-10-18 — OTA

- ArduinoOTA sem senha, hostname `M5NanoC6`, porta 3232 e anúncio mDNS para
  descoberta na Arduino IDE. Reinicia o serviço ao reconectar/trocar de IP;
  progresso e erros na Serial. OTA web mantido, sem uso simultâneo de Update.
- Barra de progresso na página OTA, com percentual do envio e estados separados
  de validação e reinício; bloqueio de envio duplicado, limite no navegador e
  mensagens de falha com possibilidade de nova tentativa.
- Testes locais aprovados: acesso sem senha, falta de partição, limite de tamanho,
  imagem inválida, envio abortado e reinício após sucesso; painel e tabela de
  partições verificados. Compilação final e gravação serão realizadas pelo usuário.
- Bluetooth LE removido da compilação, comandos e painel; canais UDP e Serial mantidos.
- Página `/ota` para upload sem senha do binário de aplicação usando Update do ESP32.
- OTA com acesso direto pela rede, sem usuário/senha, conforme solicitado.
- Validação do arquivo e do espaço; envio interrompido/ inválido não reinicia o dispositivo.
- Tabela com duas partições OTA de 1.966.080 bytes; primeira migração por USB.
- Painel e API exibem disponibilidade/limite OTA; porta independente das métricas preservada.
- Script CMD ajustado ao limite da nova tabela e documentação de instalação/atualização.


## Revisão documental — 2026-10-18

- Documentado `NAME`, que retorna `M5NanoC6` para identificar a placa.
- Catálogo geral sincronizado com `LCD_ON`/`LCD_OFF`, botão GPIO9,
  Serial/UDP/BLE e ausência de ArduinoOTA no firmware atual.
- Explicitado o padrão de porta 80 para `/metrics`, independente do padrão
  9120 adotado por outros projetos.
- Preservados os registros de compilação e testes de etapas anteriores;
  não houve nova compilação ou teste físico nesta revisão.
- [Histórico consolidado dos firmwares](../../ESP32_Documents/changelog.md).

## 2026-10-16

- Porta nas preferências configura somente a coleta `/metrics`, com aplicação
  após salvar e restauração no boot/AP. Painel, API e configuração permanecem
  na porta 80, sem redirecionamento. Porta de métricas padrão 80; valor salvo
  é preservado. Ao escolher 80, as métricas compartilham o servidor do painel.
- YAML gerado usa a porta configurada do ESP32; documentação distingue essa
  porta da porta de acesso ao servidor Prometheus.
- Testes para porta padrão, persistência no YAML, link de métricas na porta
  configurada e formulário sem redirecionamento do painel.
- Endpoint `/metrics` no formato Prometheus 0.0.4 para coleta direta pelo servidor.
- Página `/prometheus` com endereço, porta e intervalo persistidos em Preferences,
  validação de campos, API de configuração e download de `prometheus.yml`.
- Métricas reais de identificação, sistema, Wi-Fi, CPU, memória, flash,
  GPIOs já configurados, HTTP e clima externo identificado como Open-Meteo.
- Contadores persistentes de inicializações e watchdog, gravados uma vez por boot.
- Amostragem de CPU a partir dos tempos reais das tarefas/idle, com tratamento
  de wrap dos contadores; middleware mede requisições, erros e duração HTTP.
- Métricas sem hardware/recurso instalado são omitidas e documentadas em
  `PROMETHEUS.md`, sem valores fictícios de energia, bateria, relés ou MQTT.
- Testes nativos do serviço e testes do formulário web aprovados: persistência
  simulada, falhas NVS, boot/watchdog, CPU com wrap, contadores, labels,
  exposição, parâmetros do formulário, recarga e erros de comunicação.
- Documentação revisada: rotas web, Serial, configuração da coleta, métricas
  indisponíveis e verificação de espaço na flash pela saída da compilação.
- Fórmulas de uso/espaço livre na partição e exemplo PowerShell para medir
  o binário de aplicação, com distinção entre tamanho de fonte e firmware.

## 2026-10-15

- Botão integrado GPIO9 alterna o LCD, com filtro de contato de 35 ms e uma
  ação por pressionamento, acompanhando o estado definido pelos demais canais.

- Comandos `LCD_ON` e `LCD_OFF` por UDP, Serial e Bluetooth LE.
- Botões Ligar tela e Desligar tela no painel e rotas POST `/lcd_on` e `/lcd_off`.
- Controle conjunto do conteúdo e backlight, sem interromper os demais serviços;
  atualização imediata ao religar e inicialização sempre com a tela ligada.
- Estado `lcd.ligado` na API e no painel; falhas I2C retornam erro.
- Recepção Serial não bloqueante a 115200 baud, linhas LF/CRLF, limite de 255
  caracteres ASCII; respostas no próprio canal Serial.
- Validação: compilação aprovada para NanoC6/Huge APP (1.758.398 bytes, 55%;
  memória global 48.920 bytes, 14%). Testes do painel passaram, incluindo
  rotas POST de ligar/desligar e atualização do estado/prévia do LCD.
  Testes físicos do botão, LCD e entrada Serial dependem de gravação por USB.

## 2026-10-13

### Adicionado

- LCD 20x4 com adaptador PCF8574T: SDA no G2 e SCL no G1, I2C a 100 kHz.
- Detecção automática de um único dispositivo entre 0x20 e 0x27; endereço
  explícito opcional em `Config::LCD_ENDERECO`.
- Tela com data/hora, IP, máxima/mínima, umidade e temperatura/tempo de Porto Alegre,
  atualizada a cada segundo sem limpar a tela inteira. Em modo AP, mostra o IP do portal.
- Driver local usando `Wire`, sem instalar uma biblioteca LCD adicional.
- Logs de ausência/falha do LCD; os demais serviços continuam funcionando.
- Última linha substitui o estado NTP por temperatura externa e condição
  meteorológica de Porto Alegre, RS, via Open-Meteo. Consulta HTTPS com
  certificados validados em tarefa separada, a cada 10 minutos; após falha,
  repete em 2 minutos e sinaliza dado anterior com `*` por até 30 minutos.
- Dependência ArduinoJson 7 para interpretar os dados meteorológicos.
- Bluetooth LE com nome `M5NanoC6` e perfil Nordic UART: mesmos comandos
  por BLE e UDP, com resposta pelo canal de origem, inclusive no modo AP.
- RX com linhas terminadas em LF/CRLF, fragmentação, limite de 255 bytes e
  fila de quatro comandos. TX por notificações de até 20 bytes, sem bloquear
  o loop para enviar respostas longas; sessão anterior descartada ao desconectar.
- Retomada automática do anúncio BLE após desconexão e instruções em `COMANDOS.md` e `HELP`.
- Leitura de data/hora diretamente do relógio, sem espera nem dependência
  de uma janela de timeout zero durante a atualização do LCD.

- LCD sem nome da rede: IP sobe à segunda linha; terceira mostra máxima e mínima de hoje em °C e umidade em %. Quarta usa até 20 colunas para temperatura atual e condição, sem cortar palavras.
- Previsão diária do Open-Meteo com dia definido pelo fuso de Porto Alegre; máxima/mínima do dia anterior deixam de aparecer após a meia-noite.
- Painel web com atualização automática, clima detalhado, LCD, Bluetooth, rede, memória, hardware e controles. API de estado não expõe senhas; medição Wi-Fi apenas sob solicitação.
- Partição Huge APP de 3 MB, necessária para Bluetooth e HTTPS; arquivo partitions.csv incluído.

### Removido

- ArduinoOTA, inicialização OTA e processamento OTA no loop. Atualizações
  do firmware passam a ser feitas por USB.

### Validação

- Testes do JavaScript do painel passaram: dados, portal AP, clima anterior, LCD ausente, SSID especial, LED, fuso/DST, perda de contato e rotas.
- Consulta HTTPS real confirmou temperatura, umidade, vento, sensação térmica e previsão diária de máxima/mínima para Porto Alegre.
- Compilação final aprovada para M5NanoC6 com Arduino ESP32 3.3.12 e partição Huge APP: 1.754.106 bytes de programa (55% de 3 MB), 48.832 bytes de memória global (14%). A partição padrão não comporta o firmware.
- Testes no LCD e no celular BLE ainda dependem de gravação por USB. Prévia visual em navegador indisponível no ambiente.

## 2026-10-12

### Corrigido

- O serviço NTP não era iniciado no fluxo de conexão Wi-Fi. Agora inicia após
  conectar e também após recuperar uma conexão perdida, com fuso e DST salvos.
- A confirmação de sincronização usa a notificação de resposta SNTP, em vez
  de considerar uma hora previamente válida como prova de uma nova sincronização.
- Removida a espera bloqueante de inicialização NTP. O loop acompanha o resultado
  com prazo de 30 segundos e nova tentativa após mais 60 segundos em caso de falha.
- Comandos agora aceitam maiúsculas e minúsculas, conforme a lista de ajuda.
- DST manual aplica uma hora extra sem depender de regras sazonais POSIX implícitas.

### Adicionado

- `NTP_SYNC` para solicitar nova sincronização e `NTP_STATUS` para consultar o resultado.
- Novos comandos na resposta de `HELP` e documentação completa em `COMANDOS.md`.
- Inventário geral de comandos atualizado com os recursos NTP do M5NanoC6.
- Estado `SINCRONIZANDO` no comando `STATUS` e no painel web.
- Logs de início, sucesso e falha NTP com orientação sobre Internet, DNS e UDP 123.

### Validação

- Tentativa de compilação para M5NanoC6 com Arduino ESP32 3.3.12 interrompida
  pela demora excessiva do ambiente, sem resultado final. Compilação pendente.
- Sincronização real depende de validação no dispositivo com acesso à Internet.
