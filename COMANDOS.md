# Comandos do M5NanoC6

Revisão documental: **31/10/2026**.
[Histórico local](CHANGELOG.md) e
[catálogo geral](../../ESP32_Documents/COMANDOS.md).

## Home Assistant

| Comando UDP/Serial | Função |
| --- | --- |
| `HA_USER` | Retorna o usuário MQTT cadastrado. |
| `HA_PASSWORD` | Retorna a senha MQTT cadastrada em texto. |
| `HA_STATUS` | Mostra ativação, Wi-Fi/MQTT, broker, código de conexão e andamento do discovery. |
| `HA_DISCOVERY` | Agenda a republicação das 12 entidades; solicita conexão se necessário. |

As respostas voltam pelo mesmo canal. Sem credencial salva, informa que não
está cadastrada. Aceita também `ha_user` e `ha_password` em minúsculas.

Abra `/homeassistant` ou o botão **Home Assistant** no painel. Configure
broker MQTT, porta (padrão 1883), usuário/senha opcionais, intervalo e ative
a integração. O endereço é o do **broker**, que pode ser diferente do servidor
Home Assistant. A descoberta usa MQTT, não a porta ArduinoOTA.
Veja [HOME_ASSISTANT.md](HOME_ASSISTANT.md) para instalar e usar.

## Prometheus

O endpoint `/metrics` também publica estado do listener UDP e contadores de
pacotes recebidos, respostas enviadas e erros de envio. Veja os nomes e
critérios de contagem em [PROMETHEUS.md](PROMETHEUS.md#monitoramento-udp).

No painel web, abra **Prometheus** (`/prometheus`) para salvar o endereço do
servidor, porta e intervalo nas preferências. A coleta é feita pelo servidor
em `/metrics`; copie a configuração gerada para o Prometheus. A porta da
coleta no ESP32 é configurável; painel e API permanecem na porta 80.
O padrão de coleta deste projeto é **80**, não 9120. Uma porta já salva
é preservada; não aplique o padrão de outra placa sobre uma configuração existente.
Consulte [PROMETHEUS.md](PROMETHEUS.md) para
ativação, nomes das métricas e recursos ainda indisponíveis.

## Canais de comandos

Envie os comandos por **UDP** (IP do dispositivo, porta **4210**) ou pela
**Serial**, a 115200 baud. Bluetooth está desativado nesta versão.
As respostas retornam pelo canal de origem. Em UDP, voltam ao IP e à porta
do remetente. Comandos aceitam maiúsculas
ou minúsculas e espaços nas extremidades. A Serial também aceita os comandos
a 115200 baud, com final de linha LF ou CRLF, e mostra respostas e logs.

Na Serial, selecione **Nova linha** ou **Ambos NL e CR** no Monitor Serial.
O limite é de 255 caracteres ASCII por linha. Linhas inválidas ou longas
são descartadas até a próxima quebra de linha; a leitura não espera o comando
ficar completo. Exemplos: `HELP`, `LCD_OFF`, `LCD_ON`, `NTP_STATUS`.
A configuração de Prometheus é feita pela web, sem novos comandos de configuração
UDP/Serial.

A resposta de `HELP` começa com `COMANDOS` e lista um comando por linha,
com argumentos após `:`. Descrições e instruções de transporte ficam fora da
lista para facilitar o parseamento automático.

## Ligar e desligar a tela

| Comando | Função |
| --- | --- |
| `LCD_ON` | Liga o conteúdo e a iluminação do LCD; atualiza as informações. |
| `LCD_OFF` | Desliga o conteúdo e a iluminação do LCD. Os demais serviços continuam. |

Envie por UDP na porta 4210 ou pelo Monitor Serial a 115200 baud (LF/CRLF).
No painel `/info`, use **Ligar tela** e
**Desligar tela**. Para acesso HTTP direto, envie POST para `/lcd_on` ou
`/lcd_off`; retorna 503 se o LCD estiver ausente ou com falha I2C.
O estado aparece no painel e em `/api/status`, campo `lcd.ligado`.
A tela volta ligada após reiniciar; esse ajuste não é salvo na memória.
Desligar a tela não corta a alimentação elétrica do adaptador.

O botão integrado do NanoC6 ([GPIO9](https://docs.m5stack.com/en/core/NanoC6))
tem três funções: com a tela ligada, um toque alterna entre início e rede;
segurar por 3 segundos desliga conteúdo e iluminação; com a tela desligada,
um toque religa na tela inicial. O toque que religa não alterna a página.
A tela de rede retorna ao início após 15 segundos sem interação no botão.
A ação de toque ocorre ao soltar; manter pressionado não repete ações.
O filtro de 35 ms evita acionamentos duplicados por oscilação do contato.
O botão acompanha o estado atual, inclusive após comandos pela web, UDP ou Serial.

## Atualização OTA

Também é possível gravar pela **porta de rede da Arduino IDE**, anunciada
como `M5NanoC6` via mDNS, com ArduinoOTA na porta 3232 e sem senha.
Instale esta versão antes de procurar a porta. Consulte [OTA.md](OTA.md)
para instalação, descoberta e verificação do firewall.

Bluetooth foi desativado para liberar flash e RAM. Os comandos continuam
pela web, UDP e Serial. Para atualizar o firmware, abra **Atualizar firmware**
no painel ou `/ota` (porta 80), com acesso direto **sem senha**.

A primeira instalação com a nova tabela de partições precisa ser por USB,
sem apagar toda a flash se quiser preservar as prefs. Depois, envie somente
`build/M5NanoC6_wifi.ino.bin` pela página OTA. Consulte [OTA.md](OTA.md).

## Sistema

| Comando | Função |
| --- | --- |
| `HELP` | Lista os comandos disponíveis. |
| `NAME` | Identifica o dispositivo; responde `M5NanoC6`. |
| `INFO` | Informações gerais, rede, data, hora e memória. |
| `STATUS` | Estado do Wi-Fi, NTP e heap. |
| `VERSION` | Versão gerada pela data e hora de compilação. |
| `BUILD` | Data e hora de compilação. |
| `REASON` | Motivo do último reset e uptime. |
| `ALIVE` | Responde `alive`. |
| `REBOOT` | Reinicia o dispositivo. |
| `RESET_COUNTERS` | Zera somente os contadores NVS persistentes de boot/watchdog; preserva as configurações Prometheus. |
| `RESET_WIFI` | Apaga todas as preferências, incluindo redes, fuso e DST, e reinicia. |

## Hora e sincronização

| Comando | Função |
| --- | --- |
| `NTP_SYNC` | Reinicia a consulta NTP usando o fuso e DST salvos. Responde imediatamente com `SINCRONIZANDO` ou falha por ausência de Wi-Fi. |
| `NTP_STATUS` | Mostra `SINCRONIZANDO`, `OK` ou `FALHA`, além de data e hora. |
| `TIME` | Hora local. |
| `DATE` | Data local. |
| `SET_FUSO:<n>` | Salva o deslocamento em horas em relação ao UTC. Exemplo: `SET_FUSO:-3` para Brasília. Use um inteiro de -12 a 14. |
| `DST_ON` | Salva e aplica uma hora adicional de horário de verão manualmente. |
| `DST_OFF` | Desativa a hora adicional. |

O NTP inicia automaticamente após cada conexão Wi-Fi. Servidores:
`a.st1.ntp.br`, `pool.ntp.org` e `time.nist.gov`.
A confirmação exige uma resposta SNTP recebida; uma hora previamente válida
não confirma uma nova tentativa. O prazo inicial é de 30 segundos, sem bloquear
o loop. Se não chegar resposta, há nova tentativa 60 segundos após esse prazo;
uma resposta tardia também é aceita. Após sucesso, o SNTP mantém suas atualizações
periódicas. Durante uma tentativa, `TIME` e `DATE` podem mostrar a hora anterior.

Para verificar: envie `NTP_SYNC`, aguarde alguns segundos e envie `NTP_STATUS`.
Se aparecer `FALHA`, confira acesso à Internet, DNS e liberação de UDP 123.
O portal de configuração em modo AP não fornece conexão com a Internet.

## Hardware e rede

| Comando | Função |
| --- | --- |
| `TEMP` | Temperatura da CPU. |
| `CPU` | Modelo, revisão, núcleos e frequência. |
| `RAM` | Uso e disponibilidade da RAM. |
| `HEAP` | Heap livre em KB. |
| `FLASH` | Informações de memória flash e sketch. |
| `UPTIME` | Tempo ligado em segundos. |
| `NET_INFO` | SSID, IP, gateway, máscara, DNS e RSSI. |
| `MAC` | Endereço MAC. |
| `RSSI` | Sinal Wi-Fi em dBm. |
| `IP` | IP local. |

## LED e infravermelho

| Comando | Função |
| --- | --- |
| `LED_ON` | Liga o LED RGB em branco. |
| `LED_OFF` | Apaga o LED RGB. |
| `LED_BLINK[:ms]` | Pisca em branco; padrão 1000 ms, mínimo 50 ms. |
| `SET_RGB:R,G,B` | Cor RGB, valores de 0 a 255. Exemplo: `SET_RGB:255,0,0`. |
| `SET_BREATH:R,G,B,MS` | Efeito de respiração; ciclo mínimo 100 ms. Exemplo: `SET_BREATH:0,255,255,2000`. |
| `IR_TX` | Emite um pulso de infravermelho. |

## LCD 20×4

O display PCF8574T usa SDA=G2 e SCL=G1. Para LCD alimentado em 5 V com
pull-ups para 5 V, use conversor bidirecional I2C 3,3 V/5 V com terra comum.
O driver assume o backpack comum: P0=RS, P1=RW, P2=EN, P3=backlight,
P4..P7=D4..D7. Não é necessária uma biblioteca LCD adicional.

O endereço é detectado entre 0x20 e 0x27. Se houver mais de um dispositivo
nessa faixa, defina o endereço do LCD em `Config::LCD_ENDERECO`.
Os logs Serial a 115200 baud mostram o endereço encontrado ou a falha.
Ausência do LCD não impede o funcionamento do firmware.

O LCD possui duas telas. Ao ligar/religar, abre sempre a tela inicial.

| Linha | Tela inicial | Tela de rede |
| --- | --- | --- |
| 1 | Ícone de calendário, data e hora local | Ícone de rede e estado Wi-Fi/AP |
| 2 | Ícones de máxima/mínima e valores em °C | IP da estação ou portal AP |
| 3 | Temperatura atual em °C e `Umid:80%` | Nome da rede (SSID) |
| 4 | Ícone e descrição completa do clima | RSSI em dBm e canal Wi-Fi |

Os ícones são caracteres personalizados do HD44780, sem biblioteca adicional.
Descrições meteorológicas e nomes de rede longos rolam nas 20 colunas após
uma pausa inicial de 2 segundos. A tela de rede volta ao início após 15 s
sem interação no botão. Umidade continua disponível também no painel/HA.

A tela atualiza a cada segundo durante o loop normal; durante busca/conexão
Wi-Fi bloqueante, mantém a última tela e o atendimento do botão pode atrasar.
Caracteres fora do ASCII e dos ícones definidos aparecem como `?`. A prévia
web representa os ícones por símbolos Unicode, com `lcd.tela` igual a 1 ou 2
na API de status. Antes de ter hora válida, aparecem `00/00/0000` e `00:00:00`.
Se acender sem letras, ajuste o contraste do adaptador. Falha I2C desativa
as atualizações até reiniciar.

### Clima de Porto Alegre

Fonte: [Open-Meteo](https://open-meteo.com/en/docs), condições atuais estimadas
por modelos meteorológicos, não um sensor de temperatura instalado na placa.
A consulta HTTPS usa validação de certificados e começa após o relógio ter hora
válida. Atualiza a cada 10 minutos, em tarefa separada do loop; após falha,
tenta novamente em 2 minutos. Requer ArduinoJson 7 (instalado: 7.4.3) e
Arduino ESP32 3.3.12, com pacote de certificados habilitado.

Na tela inicial, máxima e mínima são previsões para o dia de Porto Alegre;
temperatura e umidade são condições atuais externas. Valores ausentes aparecem
como `--`. A umidade fica na terceira linha junto da temperatura atual;
a quarta linha é dedicada à descrição do clima. Um `*` na terceira linha
indica consulta anterior após falha ou queda do Wi-Fi, conservada por até
30 minutos. Sem dados válidos, a descrição informa buscando, sem Wi-Fi,
aguardando hora ou indisponível. As coordenadas ficam em `Config.h`.
O estado NTP continua disponível pelos comandos `NTP_STATUS` e `STATUS` e
pelo painel web.

## Acesso ao painel web

Abra `http://<IP_DO_DISPOSITIVO>/info` (porta fixa 80). Em `/prometheus`, configure somente a porta de coleta de `/metrics`, salva nas prefs; painel e API permanecem na porta 80, inclusive no portal AP. O painel atualiza a cada 5 segundos e mostra clima, máxima/mínima, umidade, sensação térmica, vento, relógio/NTP, rede, OTA, LCD, memória e informações da placa. Permite sincronizar NTP, ajustar fuso, controlar LED/IR e medir sinais das redes salvas. A medição Wi-Fi é manual e pode pausar o atendimento por alguns segundos. O estado em JSON está em `/api/status`, sem senhas.

## Compilação e atualização

A tabela `partitions.csv` do sketch reserva duas aplicações de **1.966.080
bytes** cada (1,875 MiB), além de NVS, dados OTA, SPIFFS e coredump.
Use o script na raiz para compilar com o limite correto:

```bat
cd /d C:\PC\ESP32\M5NanoC6
compilar.cmd
compilar.cmd size
```

Com GNU Make, use `make` ou `make size`. A compilação incremental preserva
os objetos. O script usa a placa M5NanoC6 e a tabela customizada do sketch,
com `upload.maximum_size=1966080`. Não selecione Huge APP para novas gravações.
O Arduino IDE pode mostrar o limite da opção de menu em vez do CSV; confira
sempre o limite real de 1.966.080 bytes por aplicação. Mais detalhes em [OTA.md](OTA.md).

### Como verificar se cabe na flash

No Arduino IDE, use **Verificar/Compilar**. No final, compare os bytes usados
com o máximo informado, que é a capacidade da **partição de aplicação**,
não toda a flash do chip. Nesta versão, cada partição OTA reserva 1.966.080 bytes.

Exemplo da versão aprovada em 15/10/2026, anterior à integração Prometheus:

```text
Sketch uses 1758398 bytes (55%) of program storage space. Maximum is 3145728 bytes.
Global variables use 48920 bytes (14%) of dynamic memory.
```

O firmware cabe quando o primeiro número é menor ou igual ao máximo e a
compilação termina sem erro. `Sketch too big` ou `text section exceeds available
space` indicam que ultrapassou a partição selecionada. A linha de variáveis
globais se refere à **RAM**, não à flash. Cada alteração exige nova compilação
para conferir o tamanho atual. Uma compilação aprovada não substitui o teste
de uso de memória durante o funcionamento na placa.

### Calcular uso e espaço livre

```text
Uso (%) = bytes do firmware / bytes da partição × 100
Livre (bytes) = bytes da partição − bytes do firmware
```

Com os números do exemplo anterior: `1.758.398 / 3.145.728 × 100 ≈ 55,9%`,
restando `1.387.330 bytes`, aproximadamente `1,32 MiB`. O percentual inteiro
mostrado pelo compilador pode ser truncado; compare também os bytes.

Também é possível usar o tamanho do arquivo de aplicação `.ino.bin`, que
inclui seus cabeçalhos e alinhamento. Após a compilação com `--build-path build`,
execute na raiz do projeto pelo PowerShell:

```powershell
$firmware = (Get-Item -LiteralPath '.\build\M5NanoC6_wifi.ino.bin').Length
$particao = 1966080
'Uso: {0:N2}%' -f (100 * $firmware / $particao)
'Livre: {0} bytes' -f ($particao - $firmware)
```

O tamanho do código-fonte `.ino` não permite prever com precisão o tamanho
final: bibliotecas, otimizações e seleção de funções pelo linker interferem.
Use sempre o resultado ou o binário da versão que acabou de compilar.

## Rotas da interface web

| Método e rota | Função |
| --- | --- |
| `GET /info` | Painel do dispositivo. |
| `GET /ota` | Página de atualização, sem senha. |
| `POST /ota` | Recebe firmware de aplicação sem senha e reinicia após validar. |
| `GET /config` | Cadastro das redes Wi-Fi. |
| `POST /salvar` | Salva as redes e reinicia. |
| `GET /api/status` | Diagnóstico em JSON, sem senhas. |
| `POST /lcd_on`, `POST /lcd_off` | Liga/desliga conteúdo e iluminação do LCD. |
| `GET /led_on`, `GET /led_off`, `GET /ir_tx` | Controles existentes de LED e IR. |
| `POST /ntp_sync` | Inicia sincronização de hora. |
| `POST /set_fuso` | Salva `fuso` e, opcionalmente, `dst=0/1`. |
| `POST /scan_wifi` | Mede sinais das redes salvas. |
| `GET /reboot` | Reinicia o dispositivo. |
| `GET /prometheus` | Configuração web da coleta Prometheus. |
| `GET /metrics` | Métricas no formato Prometheus 0.0.4. |
| `GET /prometheus.yml` | Download da configuração de coleta. |
| `GET /api/prometheus/config` | Lê as preferências e o YAML gerado. |
| `POST /api/prometheus/config` | Salva `host`, `port` e `interval`, como formulário. |
