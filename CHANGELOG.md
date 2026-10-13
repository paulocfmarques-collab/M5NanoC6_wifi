# Changelog

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
