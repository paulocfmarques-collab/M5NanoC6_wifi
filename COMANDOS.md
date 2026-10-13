# Comandos do M5NanoC6

Envie um comando por datagrama UDP ao IP do dispositivo, porta **4210**.
As respostas retornam ao IP e à porta de origem. Comandos aceitam maiúsculas
ou minúsculas e espaços nas extremidades. Não há entrada de comandos pela
Serial; ela exibe os logs a 115200 baud.

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

## Painel web

Abra `http://<IP_DO_DISPOSITIVO>/info` para consultar o estado NTP e os demais
diagnósticos. Atualize a página para acompanhar a sincronização.
