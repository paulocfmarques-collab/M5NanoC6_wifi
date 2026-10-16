# Comandos do M5NanoC6

## Prometheus

No painel web, abra **Prometheus** (`/prometheus`) para salvar o endereço do
servidor, porta e intervalo nas preferências. A coleta é feita pelo servidor
em `/metrics`; copie a configuração gerada para o Prometheus. A porta da
coleta no ESP32 é configurável; painel e API permanecem na porta 80.
Consulte [PROMETHEUS.md](PROMETHEUS.md) para
ativação, nomes das métricas e recursos ainda indisponíveis.

## Canais de comandos

Envie os comandos por **UDP** (IP do dispositivo, porta **4210**) ou por
**Bluetooth LE**, conectando ao dispositivo **M5NanoC6**.
As respostas retornam pelo canal de origem. Em UDP, voltam ao IP e à porta
do remetente. Comandos aceitam maiúsculas
ou minúsculas e espaços nas extremidades. A Serial também aceita os comandos
a 115200 baud, com final de linha LF ou CRLF, e mostra respostas e logs.

Na Serial, selecione **Nova linha** ou **Ambos NL e CR** no Monitor Serial.
O limite é de 255 caracteres ASCII por linha. Linhas inválidas ou longas
são descartadas até a próxima quebra de linha; a leitura não espera o comando
ficar completo. Exemplos: `HELP`, `LCD_OFF`, `LCD_ON`, `NTP_STATUS`.
A configuração de Prometheus é feita pela web, sem novos comandos de configuração
UDP/BLE/Serial.

## Ligar e desligar a tela

| Comando | Função |
| --- | --- |
| `LCD_ON` | Liga o conteúdo e a iluminação do LCD; atualiza as informações. |
| `LCD_OFF` | Desliga o conteúdo e a iluminação do LCD. Os demais serviços continuam. |

Envie por UDP na porta 4210 ou pelo Monitor Serial a 115200 baud (LF/CRLF).
Também funcionam pelo Bluetooth LE. No painel `/info`, use **Ligar tela** e
**Desligar tela**. Para acesso HTTP direto, envie POST para `/lcd_on` ou
`/lcd_off`; retorna 503 se o LCD estiver ausente ou com falha I2C.
O estado aparece no painel e em `/api/status`, campo `lcd.ligado`.
A tela volta ligada após reiniciar; esse ajuste não é salvo na memória.
Desligar a tela não corta a alimentação elétrica do adaptador.

O botão integrado do NanoC6 ([GPIO9](https://docs.m5stack.com/en/core/NanoC6))
também alterna a tela: um toque desliga, outro liga. Segurar não repete a ação.
O filtro de 35 ms evita acionamentos duplicados por oscilação do contato.
O botão acompanha o estado atual, inclusive após comandos pela web, UDP ou Serial.

## Bluetooth LE

O ESP32-C6 usa BLE; conecte por um aplicativo compatível com o perfil
[Nordic UART Service](https://nrfconnectdocs.nordicsemi.com/ncs/latest/nrf/libraries/bluetooth/services/nus.html).
A conexão é feita no aplicativo, sem PIN de pareamento.

| Item | UUID |
| --- | --- |
| Serviço Nordic UART | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| RX: escrever comandos | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` |
| TX: receber notificações | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` |

1. Conecte a **M5NanoC6** pelo aplicativo BLE.
2. Habilite as notificações de **TX**.
3. Escreva em **RX**, como texto UTF-8/ASCII, um comando terminado em **LF**
   (quebra de linha). No terminal, selecione final de linha LF ou CRLF.
4. Comece com `HELP`, depois teste `LED_ON`, `TIME` ou `NTP_SYNC`.

Exemplo: os bytes de `LED_ON` seguidos por `0A` (LF). Escrever os dois caracteres
literais `\n` não equivale a enviar a quebra de linha.
O firmware reúne fragmentos até LF, aceita até 255 bytes ASCII por comando
e mantém uma fila de quatro linhas. Aguarde a resposta antes do próximo envio.
As respostas são notificações de até 20 bytes, enviadas em ordem: o aplicativo
deve concatená-las para mostrar mensagens longas, como `HELP` e `INFO`.
Ao desconectar, comandos antigos são descartados e o dispositivo volta a anunciar.

BLE funciona também no portal AP, sem rede Wi-Fi conectada. Data/hora e clima
continuam dependendo do acesso à Internet por Wi-Fi. A busca por redes é
bloqueante e pode atrasar o processamento dos comandos BLE/UDP.
O OTA foi removido: novas versões do firmware devem ser gravadas por USB.

## Sistema

| Comando | Função |
| --- | --- |
| `HELP` | Lista os comandos disponíveis. |
| `INFO` | Informações gerais, rede, data, hora e memória. |
| `STATUS` | Estado do Wi-Fi, NTP e heap. |
| `VERSION` | Versão gerada pela data e hora de compilação. |
| `BUILD` | Data e hora de compilação. |
| `REASON` | Motivo do último reset e uptime. |
| `ALIVE` | Responde `alive`. |
| `REBOOT` | Reinicia o dispositivo. |
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

| Linha | Conteúdo |
| --- | --- |
| 1 | Data e hora local |
| 2 | IP local ou IP do portal |
| 3 | Máxima e mínima previstas para hoje (°C) e umidade atual (%) |
| 4 | Temperatura em °C e condição do tempo em Porto Alegre, RS |

A tela atualiza a cada segundo durante o loop normal; durante busca/conexão
Wi-Fi bloqueante, mantém a última tela. Textos longos são cortados em 20
colunas; caracteres fora do ASCII aparecem como `?`. Antes de ter hora válida,
aparecem `00/00/0000` e `00:00:00`. Se acender sem letras, ajuste o potenciômetro
de contraste do adaptador. Uma falha I2C desativa a atualização até reiniciar.

### Clima de Porto Alegre

Fonte: [Open-Meteo](https://open-meteo.com/en/docs), condições atuais estimadas
por modelos meteorológicos, não um sensor de temperatura instalado na placa.
A consulta HTTPS usa validação de certificados e começa após o relógio ter hora
válida. Atualiza a cada 10 minutos, em tarefa separada do loop; após falha,
tenta novamente em 2 minutos. Requer ArduinoJson 7 (instalado: 7.4.3) e
Arduino ESP32 3.3.12, com pacote de certificados habilitado.

Na terceira linha, `Max:24 Min:16 U80%` significa máxima de 24 °C, mínima de 16 °C e umidade atual de 80%. Se necessário, os rótulos passam a `Mx` e `Mn` para caber nas 20 colunas. Máxima e mínima são previsões para o dia de Porto Alegre. Valores ausentes aparecem como `--`. Na quarta linha, a descrição usa até as 20 colunas, sem cortar palavras; a temperatura usa uma casa decimal quando couber. Exemplo: `16.8C Garoa leve`. Um `*` indica que a última
consulta falhou ou que o Wi-Fi caiu: o último resultado é conservado por até
30 minutos. Sem dados, a linha mostra o motivo: buscando, sem Wi-Fi,
aguardando hora ou indisponível. As coordenadas ficam em `Config.h`.
O estado NTP continua disponível pelos comandos `NTP_STATUS` e `STATUS` e
pelo painel web.

## Acesso ao painel web

Abra `http://<IP_DO_DISPOSITIVO>/info` (porta fixa 80). Em `/prometheus`, configure somente a porta de coleta de `/metrics`, salva nas prefs; painel e API permanecem na porta 80, inclusive no portal AP. O painel atualiza a cada 5 segundos e mostra clima, máxima/mínima, umidade, sensação térmica, vento, relógio/NTP, rede, Bluetooth, LCD, memória e informações da placa. Permite sincronizar NTP, ajustar fuso, controlar LED/IR e medir sinais das redes salvas. A medição Wi-Fi é manual e pode pausar o atendimento por alguns segundos. O estado em JSON está em `/api/status`, sem senhas.

## Compilação e atualização

Selecione a placa M5NanoC6 e a partição **Huge APP (3MB No OTA/1MB SPIFFS)**. Bluetooth e HTTPS excedem a partição padrão. O arquivo `partitions.csv` acompanha o sketch. Atualize o firmware por USB; OTA foi removido.

Com Arduino CLI:

```text
arduino-cli compile --fqbn esp32:esp32:m5stack_nanoc6:PartitionScheme=huge_app --build-path build M5NanoC6_wifi
```

### Como verificar se cabe na flash

No Arduino IDE, use **Verificar/Compilar**. No final, compare os bytes usados
com o máximo informado, que é a capacidade da **partição de aplicação**,
não toda a flash do chip. Neste projeto, Huge APP reserva 3.145.728 bytes.

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
$particao = 3145728
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
