# Prometheus no M5NanoC6

## Configuração pela web

1. Abra o painel do ESP32 e entre em **Prometheus**, ou acesse `/prometheus`.
2. Informe o endereço do servidor Prometheus (IPv4 ou DNS, sem protocolo), a
   **porta de métricas do ESP32** (padrão 80; por exemplo 9120) e o intervalo de coleta
   entre 5 e 3600 segundos.
3. Salve. Os campos são armazenados juntos em Preferences/NVS, namespace
   `telemetry`, e permanecem após reiniciar. Uma gravação com erro não altera
   a configuração ativa. A porta configurada é aplicada somente a `/metrics`
   cerca de um segundo após salvar. Painel, API e configuração continuam na
   porta 80, sem redirecionar o navegador, inclusive após reiniciar e no portal AP.
   Valores já salvos são preservados como porta de métricas.
   Se escolher 80, as métricas compartilham o servidor do painel; nas demais
   portas, o servidor de coleta atende somente `/metrics`.
4. Copie o job apresentado na tela para o arquivo do servidor Prometheus e
   recarregue a configuração desse servidor. Também pode baixar `/prometheus.yml`.

Foi escolhido o modelo de **coleta direta**: o Prometheus acessa
`http://IP_DO_ESP32:PORTA/metrics`. O ESP32 não envia dados à porta 9090, e salvar
suas preferências não edita automaticamente o servidor.
O endereço do Prometheus é uma referência persistente para essa configuração.
O intervalo salvo é usado no YAML gerado e só passa a valer após aplicar esse
YAML ao servidor. Use uma reserva DHCP para o IP do ESP32.

`RESET_WIFI` apaga as preferências de rede do namespace `m5wifi`; não apaga
as configurações Prometheus nem os contadores do namespace `telemetry`.

Exemplo — substitua pelo IP real do dispositivo:

```yaml
scrape_configs:
  - job_name: 'm5nanoc6'
    scrape_interval: 15s
    metrics_path: /metrics
    static_configs:
      - targets: ['192.168.1.42:80']
```

Se já houver `scrape_configs`, acrescente somente o item do job. Se já existir
um job chamado `m5nanoc6`, acrescente o target a ele ou escolha outro nome.
O endpoint também funciona no portal AP; para coleta contínua, conecte a placa
à rede acessível ao Prometheus.

## Métricas publicadas

Todas as séries têm o label `device`, com o MAC da interface STA, estável
mesmo no portal AP. Nomes e números seguem o formato de exposição Prometheus
0.0.4; textos são labels, com caracteres especiais escapados.

| Métricas | Origem e significado |
| --- | --- |
| `esp_info` | Valor 1, labels `chip` e `model`. |
| `esp_firmware_version` | Valor 1, label `version`. |
| `esp_ip_address` | Valor 1, labels `ip` e `interface` (`sta` ou `ap`); omitida sem IP ativo. |
| `esp_mac_address` | Valor 1, label `mac` da interface STA. |
| `esp_status_online` | 1 quando conectado ao Wi-Fi como estação; 0 caso contrário. Não detecta acesso à Internet. |
| `esp_uptime_seconds` | Tempo desde boot, com timer de 64 bits. |
| `esp_last_restart_reason` | Valor 1, label `reason` com o motivo do reset. |
| `esp_boot_count` | Total de inicializações registradas na NVS. |
| `esp_reboot_count` | Inicializações registradas menos a primeira, incluindo ciclos de energia. |
| `esp_watchdog_resets` | Inicializações cujo motivo foi watchdog. |
| `esp_wifi_rssi_dbm`, `esp_wifi_channel` | Sinal e canal da conexão atual; omitidas quando desconectado. |
| `esp_wifi_disconnect_count` | Eventos de desconexão desde boot, inclusive desconexões deliberadas e tentativas malsucedidas emitidas pelo driver. |
| `esp_wifi_tx_power` | Limite configurado de potência de transmissão em **dBm**, via driver; não é consumo elétrico nem medição de potência radiada. Omitida se a leitura falhar. |
| `esp_cpu_frequency_mhz` | Frequência atual da CPU. |
| `esp_cpu_usage_percent` | Uso no último intervalo de 5 s calculado por runtime das tarefas e idle, no NanoC6 de um núcleo. Omitida durante aquecimento, sem suporte de runtime ou se a amostra não for válida. |
| `esp_task_count`, `esp_free_rtos_tasks` | Número de tarefas FreeRTOS; dois nomes para o mesmo dado. |
| `esp_heap_free_bytes`, `esp_heap_total_bytes`, `esp_heap_minimum_free_bytes` | Heap livre, total e menor valor livre desde boot. |
| `esp_psram_free_bytes`, `esp_psram_total_bytes` | PSRAM livre e total; zero indica ausência de PSRAM disponível. |
| `esp_flash_usage_percent` | Tamanho da imagem em uso dividido pela capacidade da partição de aplicação, não pelo tamanho inteiro do chip flash. |
| `esp_cpu_temperature_celsius` | Métrica adicional da temperatura interna da CPU, quando válida. |
| `sensor_temperature_celsius`, `sensor_humidity_percent` | Condições externas de Porto Alegre via Open-Meteo, labels `source="open_meteo"`, `location="porto_alegre"`. Não são sensores locais. Omitidas quando sem dado válido. |
| `esp_weather_data_age_seconds` | Idade da última consulta meteorológica válida; adicional para avaliar atualização. |
| `gpio_digital_input` | Nível lógico bruto do botão GPIO9: 0 pressionado, 1 solto; label `pin`. |
| `gpio_digital_output` | Nível lógico instantâneo dos GPIOs 3 (IR), 7 (LED azul), 19 (alimentação RGB); label `pin`. Não mede pulsos entre coletas. |
| `application_requests_total` | Requisições HTTP concluídas desde boot, incluindo painel e coletas de métricas. |
| `application_errors_total` | Requisições HTTP concluídas com status 4xx/5xx ou sem código de resposta. |
| `application_response_time_ms` | Tempo de tratamento da última requisição HTTP concluída, incluindo envio da resposta; gauge, não média nem percentil. Não inclui leitura inicial de headers/body nem latência do cliente. |

Os contadores persistentes são gravados **uma vez por boot**, nunca a cada
coleta. Se a NVS estiver corrompida ou a gravação falhar, os contadores
persistentes são omitidos e a falha é registrada na Serial. Os contadores de
Wi-Fi e HTTP reiniciam no boot. Requisições que reiniciam imediatamente o
dispositivo não chegam ao registro de conclusão.

CPU fica disponível após duas amostras válidas de 5 segundos no loop normal.
Busca/conexão Wi-Fi bloqueante pode atrasar a amostragem e as respostas HTTP.
Clima obedece à validade de até 30 minutos do serviço meteorológico; a métrica
de idade permite definir um limite menor no servidor.

## Métricas solicitadas ainda indisponíveis

O firmware não inventa zero para grandezas sem medição ou recurso instalado.
Estas séries **não são publicadas** nesta configuração:

| Métricas | O que falta |
| --- | --- |
| `esp_voltage_v`, `esp_current_ma`, `esp_power_w` | Medidor elétrico e integração da leitura. |
| `esp_battery_percent`, `esp_battery_voltage_v` | Bateria monitorada, circuito de medição e calibração. |
| `sensor_pressure_hpa`, `sensor_air_quality` | Sensores correspondentes e seus drivers. |
| `relay_state`, `relay_activation_count`, `relay_runtime_seconds` | Relés, definição de pinos e lógica de acionamento/contagem. |
| `gpio_analog_value` | Entrada analógica definida e calibrada. GPIO1/GPIO2 continuam dedicados ao LCD I²C. |
| `mqtt_connected`, `mqtt_messages_sent`, `mqtt_messages_received`, `mqtt_publish_errors` | Cliente MQTT, broker e instrumentação; o projeto usa UDP/BLE/Serial/HTTP. |

## Diagnóstico de conexão recusada

Confira se o alvo usa o IP e a porta de métricas configurados no ESP32. Por exemplo,
com IP `192.168.0.115` e porta salva `9120`, teste
`http://192.168.0.115:9120/metrics` e use
`targets: ['192.168.0.115:9120']` no Prometheus. Grave esta versão do firmware
para ativar a porta salva apenas para a coleta. O painel continua acessível em
`http://192.168.0.115/info`, na porta 80.
Depois de mudar o alvo, recarregue ou reinicie o Prometheus para aplicar o YAML.
A porta de métricas do ESP32 é independente da porta de acesso ao servidor Prometheus.

## API e consultas

- `GET /metrics` na porta configurada: texto Prometheus, sem cache.
- As rotas abaixo ficam sempre na porta 80.
- `GET /api/prometheus/config`: `host`, `port` (métricas do ESP32),
  `active_port` (porta de métricas em uso), `web_port=80`, `interval`, `mode="pull"`,
  `scrape_config`, `persistent_counters_ok`.
- `POST /api/prometheus/config`: formulário com `host`, `port`, `interval`.
  Use `Content-Type: application/x-www-form-urlencoded`. Porta: 1..65535;
  intervalo: 5..3600 segundos. Erros de validação retornam 400; falha de
  gravação retorna 503. O sucesso retorna as preferências atuais em JSON.
- `GET /prometheus.yml`: configuração de coleta para download.

Para disponibilidade, use `up{job="m5nanoc6"}`: um dispositivo desligado não
consegue publicar `esp_status_online=0`. Exemplos de consultas:

```promql
up{job="m5nanoc6"}
esp_heap_free_bytes{job="m5nanoc6"}
rate(application_errors_total{job="m5nanoc6"}[5m])
esp_weather_data_age_seconds{job="m5nanoc6"} > 900
```

Referências: [modelo de coleta](https://prometheus.io/docs/prometheus/latest/getting_started/),
[formato de exposição](https://prometheus.io/docs/instrumenting/exposition_formats/),
[métricas indisponíveis](https://prometheus.io/docs/instrumenting/writing_clientlibs/).

Para conferir espaço na flash e gravar por USB, veja
[Compilação e atualização em COMANDOS.md](COMANDOS.md#compilação-e-atualização).
