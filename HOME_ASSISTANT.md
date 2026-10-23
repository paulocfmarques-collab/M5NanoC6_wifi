# Home Assistant

A integração usa MQTT Discovery e requer a biblioteca **PubSubClient 2.8**,
além de ArduinoJson 7. PubSubClient já está instalada neste computador;
em outra instalação, procure-a no gerenciador de bibliotecas Arduino.

## Configurar

1. Disponibilize um broker MQTT (por exemplo, Mosquitto) acessível ao ESP32.
2. No Home Assistant, configure a integração **MQTT** para esse broker.
3. Compile e grave este firmware. Se já possui OTA, pode atualizar por ele;
   a tabela de partições OTA não mudou. Confira que o novo binário cabe no slot.
4. Abra `http://IP_DO_ESP32/homeassistant`, na porta 80 do painel.
5. Ative a integração, informe o **endereço do broker MQTT**, porta, credenciais
   opcionais e intervalo de publicação (5..3600 segundos, padrão 15). Salve.
6. Atualize a página para consultar a conexão. No Home Assistant, o dispositivo
   **M5NanoC6** aparecerá na integração MQTT após receber a descoberta.

Broker MQTT e Home Assistant podem estar em máquinas diferentes. Não use a
porta web 8123 do Home Assistant: o padrão MQTT é **1883**. Esta implementação
usa MQTT sem TLS na rede local. O prefixo Discovery é o padrão `homeassistant`.
Um prefixo personalizado no HA precisa ser ajustado para esse padrão.

Configuração salva no namespace `m5ha` das Preferences/NVS, em um registro.
Uma falha de gravação preserva a configuração anterior. Senha vazia no
formulário mantém a salva; marque **Apagar a senha salva** para removê-la.
Ela não é retornada pela API nem exibida nos logs. Credenciais são guardadas
nas prefs, como as redes Wi-Fi; o formulário é servido pelo HTTP local existente.
`RESET_WIFI` não apaga a configuração Home Assistant.

## Entidades

| Entidade | Comportamento |
| --- | --- |
| Tela LCD | Switch ON/OFF do conteúdo e backlight. |
| LED RGB | Switch branco ligado/apagado, como `LED_ON`/`LED_OFF`. |
| Sincronizar NTP | Botão equivalente a `NTP_SYNC`. |
| Pulso infravermelho | Botão equivalente a `IR_TX`. |
| Temperatura CPU | Sensor interno, °C. |
| Memória livre | Heap livre, bytes. |
| Tempo ligado | Uptime, segundos. |
| Sinal Wi-Fi | RSSI, dBm. |
| Temperatura/umidade de Porto Alegre | Dados externos Open-Meteo, °C e %. |
| Máxima/mínima de Porto Alegre | Previsão diária Open-Meteo, °C. |

O clima não representa sensores instalados. Valores indisponíveis/vencidos
são enviados como `null` e suas entidades ficam indisponíveis por template.
O estado do LCD ausente também é `null`. O LED informa o estado configurado,
não cada pulso de um efeito. Comandos web, Serial, UDP e botão físico aparecem
no próximo estado periódico. Comandos MQTT solicitam estado imediato.

As entidades pertencem ao mesmo dispositivo com identificador estável
`m5nanoc6_<MAC sem separadores>`, sem conflitar com outras placas. Os tópicos:

```text
homeassistant/<componente>/<id>/<entidade>/config  (discovery retido)
<id>/availability                               (online/offline retido)
<id>/state                                      (JSON, não retido)
<id>/lcd/set                                    (ON ou OFF)
<id>/led/set                                    (ON ou OFF)
<id>/ntp/set                                    (PRESS)
<id>/ir/set                                     (PRESS)
```

Não retenha comandos no broker. O discovery configura comandos não retidos.
O firmware só executa os quatro controles listados; não aceita comandos
arbitrários, reboot ou configurações por MQTT.

Last Will informa `offline` em perda inesperada de conexão. Ao desativar ou
trocar broker, tenta publicar `offline` antes de desconectar. O broker decide
quando detectar a queda; não é uma indicação instantânea de falta de energia.
Discovery é republicado na conexão e em `homeassistant/status = online`,
para suportar reinício do HA. Desativar não apaga as entidades retidas:
elas ficam indisponíveis; remova-as no broker/HA se quiser excluir a integração.

Reconexão é tentada a cada 30 s, sem loop de espera. Cada tentativa de rede
pode atrasar o atendimento (timeout de conexão TCP ~1 s e MQTT ~1 s; DNS e
rede podem acrescentar demora). Discovery é enviado gradualmente. Publicações
usam QoS 0; não há confirmação de processamento dos comandos pelo HA.

## API e diagnóstico

Comandos de consulta por UDP 4210 ou Serial 115200 (LF/CRLF), com resposta
no canal de origem:

| Comando | Resposta |
| --- | --- |
| `HA_USER` | `HA_USER: <usuário MQTT salvo>`; informa se não cadastrado. |
| `HA_PASSWORD` | `HA_PASSWORD: <senha MQTT salva>`; informa se não cadastrada. |
| `HA_STATUS` | Ativação, Wi-Fi, MQTT/código, broker/porta, ID, intervalo e andamento do discovery; não inclui senha. |
| `HA_DISCOVERY` | Agenda republicação das 12 configurações retidas e do estado; retorna `AGENDADO` ou `INDISPONIVEL`. |

O discovery manual exige integração ativada. Se o broker estiver desconectado,
solicita uma tentativa de conexão e aguarda o MQTT; o comando não bloqueia a
resposta esperando o broker. `AGENDADO` não confirma conclusão. Consulte
`HA_STATUS`: `ENVIANDO n/12`, `AGUARDANDO MQTT n/12` ou `SEM ENVIO PENDENTE`.
Este último indica que não há configurações aguardando publicação local,
não que o Home Assistant confirmou a criação das entidades.

Esses comandos devolvem as credenciais MQTT em texto, conforme solicitado,
inclusive com a integração desativada. Não alteram as prefs. A API web
continua sem devolver a senha; não são comandos aceitos por MQTT.

- `GET /api/homeassistant/config`: campos públicos, `has_password`,
  `connected`, `connection_state` e `device_id`; nunca a senha.
- `POST /api/homeassistant/config`: formulário `enabled=0/1`, `host`, `port`,
  `user`, `password`, `clear_password=0/1`, `interval`. Erro de campos: 400;
  erro ao salvar: 503. Não envia a configuração ao Home Assistant por HTTP.
- `/metrics`: `mqtt_connected`, `mqtt_messages_sent`, `mqtt_messages_received`
  e `mqtt_publish_errors`, com contadores zerados no boot.

Se não conectar, confira IP/DNS do broker, firewall, porta e credenciais.
Código PubSubClient 0 indica conectado; -2 costuma indicar falha de conexão;
4/5 indicam credenciais/autorização rejeitadas. Se conectar sem aparecer no
HA, confira a integração MQTT, o prefixo Discovery e as permissões de tópicos
do usuário do broker. A senha MQTT é independente do OTA, que continua sem senha.

Referências oficiais: [MQTT e Discovery](https://www.home-assistant.io/integrations/mqtt/),
[switch MQTT](https://www.home-assistant.io/integrations/switch.mqtt/),
[sensores MQTT](https://www.home-assistant.io/integrations/sensor.mqtt/).
