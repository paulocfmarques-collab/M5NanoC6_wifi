# Atualização OTA do M5NanoC6

## Primeira instalação por USB

A versão anterior usa uma única aplicação Huge APP. Para ativar OTA é
necessário gravar por USB este firmware **e a nova tabela de partições**.
Um upload OTA não faz essa migração. Não envie um binário OTA sobre a tabela antiga.

Na raiz do projeto, pelo Prompt de Comando:

```bat
cd /d C:\PC\ESP32\M5NanoC6
compilar.cmd
```

Com GNU Make instalado, `make` chama o mesmo script. A saída fica em `build`.
Não execute duas compilações simultâneas nessa pasta. A compilação preserva
objetos para reaproveitar nas próximas execuções.

Para gravar por USB, substitua COM5 pela porta da placa (Arduino CLI no PATH):

```bat
arduino-cli upload --fqbn esp32:esp32:m5stack_nanoc6:PartitionScheme=default --port COM5 --input-dir build M5NanoC6_wifi
```

O upload deve usar os artefatos completos gerados em `build`, incluindo a tabela.
Evite a opção de apagar toda a flash para preservar redes, porta de métricas,
contadores e outras prefs. A região NVS permanece em 0x9000, com 0x5000 bytes.
O espaço de SPIFFS mudou; arquivos de uma eventual instalação anterior não
são migrados. Este projeto não usa SPIFFS para guardar as preferências.

## Gravar pela Arduino IDE (primeira vez, por USB)

1. Abra `M5NanoC6_wifi/M5NanoC6_wifi.ino` na Arduino IDE e selecione a placa
   **M5Stack NanoC6** e a porta USB da placa.
2. Em **Ferramentas → Partition Scheme**, selecione **Huge APP** se necessário
   para permitir compilar o binário atual. Embora o nome dessa opção indique
   que não há OTA, o `partitions.csv` incluído na pasta do sketch substitui a
   tabela do menu durante a compilação e instala as duas partições OTA deste
   projeto.
3. Clique em **Verificar/Compilar** e depois em **Carregar** para gravar pela
   USB. Não use “Erase All Flash” se quiser preservar as preferências.

O limite exibido pela IDE pode ser o da opção **Huge APP**, maior que o espaço
real de uma aplicação OTA. Confirme que o arquivo compilado
`M5NanoC6_wifi.ino.bin` tem no máximo **1.966.080 bytes**; o serviço OTA rejeita
imagens maiores. Depois da primeira gravação, conecte a placa ao Wi-Fi e siga
as instruções abaixo para enviar pela rede.

## Gravar pela Arduino IDE (porta de rede)

O firmware anuncia `M5NanoC6` pelo mDNS e atende ArduinoOTA na porta **3232**,
sem senha. Após instalar esta versão e conectar o ESP32 ao Wi-Fi:

1. Mantenha computador e ESP32 na mesma rede local. O computador pode usar Ethernet.
2. Na Arduino IDE, selecione a placa M5NanoC6 e abra **Ferramentas → Porta**.
3. Escolha a porta de rede `M5NanoC6` com o IP do ESP32 e faça o upload normal.

O progresso da gravação pela rede é mostrado pela própria IDE. A barra de
progresso da página `/ota` é para atualizações feitas pelo navegador.

### Falha de autenticação no core ESP32 3.3.12

Nesse core, chamar `ArduinoOTA.setPassword("")` ativa a autenticação com o
hash de uma senha vazia. Para funcionar sem senha, o firmware não chama
`setPassword` nem `setPasswordHash`.

Se a placa ainda executa a versão com essa chamada e o envio pela IDE falha
com `Authentication Failed`, instale o firmware corrigido por USB ou envie
somente o novo `M5NanoC6_wifi.ino.bin` em `http://IP_DO_ESP32/ota`, desde que
a tabela OTA já esteja instalada. Repetir o envio pela IDE não atualiza o
código que está rodando na placa.

Após reiniciar, selecione novamente a porta de rede `M5NanoC6` na IDE.
O firmware corrigido não exige senha; não use `123456`. Se a IDE continuar
pedindo senha com dados antigos de descoberta, reabra a IDE e selecione
novamente a porta de rede.

Se a versão atualmente instalada só possui OTA web, você pode enviar esta
versão por `/ota` para ativar o anúncio da IDE, desde que a tabela OTA já
esteja instalada. Caso contrário, instale primeiro por USB.

Se a porta não aparecer, aguarde a conexão Wi-Fi, reabra o menu de portas e
confira o log `[ArduinoOTA]`. Libere Arduino IDE/Python no firewall da rede
privada e permita mDNS UDP 5353 e OTA UDP 3232. O upload também abre uma
conexão TCP do ESP32 para o computador; essa conexão de retorno precisa ser
permitida. Redes de convidados, isolamento entre clientes e VLANs podem
impedir descoberta ou comunicação. O serviço da IDE é iniciado em modo
estação; no portal AP, utilize a atualização pela página web.

O progresso aparece na IDE e na Serial. A descoberta é retomada após queda
de conexão ou troca de IP. Não use os dois métodos de atualização ao mesmo tempo.
O OTA web continua disponível na porta 80; a porta de métricas é independente.

## Atualizações seguintes pela página web

1. Abra `http://IP_DO_ESP32/ota`, ou **Atualizar firmware** no painel.
   O painel/OTA permanecem na porta 80, independentemente da porta Prometheus.
2. O acesso é direto, **sem usuário e sem senha**, conforme solicitado.
3. Compile e selecione **somente** `build/M5NanoC6_wifi.ino.bin`.
   Não envie arquivos `bootloader`, `partitions`, `merged` ou imagem completa da flash.
4. Clique **Enviar e atualizar** e aguarde a confirmação. O ESP32 reinicia
   após validar o binário e volta ao painel. Mantenha a alimentação durante o processo.

A barra mostra o percentual enviado pelo navegador enquanto o ESP32 recebe
e grava os blocos. Ao concluir o envio, aguarda a validação do arquivo.
Ela fica visível desde a abertura de `/ota`, com percentual numérico e etapas
de envio, validação e reinício. Na Arduino IDE, acompanhe o progresso nativo
da IDE; a barra web corresponde aos envios iniciados pela própria página.
100% enviado ainda não confirma que o firmware foi aprovado. Após a resposta
de sucesso, a página informa o reinício e libera o link para abrir o painel.
Falhas mostram o motivo e liberam uma nova tentativa. O navegador também
bloqueia arquivos maiores que a partição e envios duplicados.

A página usa HTTP na rede local. Não são criadas credenciais nem prefs de OTA.
As prefs de Wi-Fi e Prometheus são mantidas durante as atualizações.

O envio grava na partição inativa. Tamanho excessivo, erro de escrita,
arquivo inválido ou upload abortado não provocam reinício automático.
O SDK valida a imagem antes de selecionar a próxima aplicação. Isso não é
assinatura digital nem teste funcional do firmware: envie builds deste projeto.
Não foi habilitado rollback automático após uma falha de execução do novo firmware.
Uma falha que impeça o novo firmware de iniciar pode exigir recuperação por USB.
Durante o envio, o atendimento dos demais controles pode sofrer atraso.

## Espaço disponível

| Região | Início | Tamanho |
| --- | --- | --- |
| NVS/prefs | 0x9000 | 0x5000 |
| Dados OTA | 0xE000 | 0x2000 |
| Aplicação 0 | 0x10000 | 0x1E0000 (1.966.080 bytes) |
| Aplicação 1 | 0x1F0000 | 0x1E0000 (1.966.080 bytes) |
| SPIFFS | 0x3D0000 | 0x20000 |
| Coredump | 0x3F0000 | 0x10000 |

Cada firmware precisa caber em **uma** aplicação, não na soma das duas.
`compilar.cmd size` calcula o uso do binário existente com esse limite.
O script compila com `--build-property upload.maximum_size=1966080` para
que o limite informado corresponda à tabela customizada do sketch.
Bluetooth foi removido; os controles continuam pela web, UDP 4210 e Serial.
