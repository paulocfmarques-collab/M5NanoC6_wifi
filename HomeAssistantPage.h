#ifndef HOME_ASSISTANT_PAGE_H
#define HOME_ASSISTANT_PAGE_H
const char homeAssistantPage[] PROGMEM=R"hapage(
<!doctype html><html lang="pt-BR"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Home Assistant · M5NanoC6</title><style>body{max-width:760px;margin:30px auto;padding:20px;background:#10141c;color:#eee;font:16px/1.6 system-ui}label{display:block;margin:16px 0}input,button{font:inherit;padding:9px;border-radius:7px}input:not([type=checkbox]){display:block;width:95%;background:#202735;color:#eee;border:1px solid #526078}button{background:#ff9751;border:0}a{color:#ff9751}.error{color:#ff9b9b}</style>
<a href="/info">Voltar ao painel</a><h1>Home Assistant</h1>
<p>Integração por MQTT Discovery. Configure o mesmo broker usado pela integração MQTT do Home Assistant. O endereço do broker pode ser diferente do endereço do Home Assistant.</p>
<p id="status" role="status"></p><form id="ha-form">
<label><input id="enabled" type="checkbox"> Ativar integração</label>
<label>Endereço do broker MQTT<input id="host" maxlength="253" placeholder="192.168.0.10 ou mqtt.local"></label>
<label>Porta MQTT (sem TLS)<input id="port" type="number" min="1" max="65535" value="1883" required></label>
<label>Usuário (opcional)<input id="user" maxlength="64" autocomplete="off"></label>
<label>Senha MQTT<input id="password" type="password" maxlength="128" autocomplete="new-password" placeholder="Em branco mantém a senha salva"></label>
<label><input id="clear-password" type="checkbox"> Apagar a senha salva</label>
<label>Intervalo de publicação (segundos)<input id="interval" type="number" min="5" max="3600" value="15" required></label>
<button id="save" disabled>Salvar nas preferências</button></form>
<p id="connection"></p><p id="identity"></p>
<p>Entidades: LCD, LED RGB, sincronização NTP, pulso IR, temperatura CPU, memória livre, uptime, RSSI e temperatura/umidade/máxima/mínima de Porto Alegre. O clima vem do Open-Meteo. A descoberta usa o prefixo padrão <code>homeassistant</code>.</p>
<p>O Home Assistant precisa da integração MQTT e de um broker, como Mosquitto. Não é necessário token de acesso ao Home Assistant.</p>
<script>
const el=id=>document.getElementById(id);
function feedback(message,error=false){el('status').textContent=message;el('status').className=error?'error':''}
function show(data){for(const key of ['host','port','user','interval'])el(key).value=data[key];el('enabled').checked=data.enabled;el('password').value='';el('clear-password').checked=false;el('connection').textContent=data.connected?'MQTT conectado.':'MQTT desconectado. Código: '+data.connection_state;el('identity').textContent='Dispositivo: '+data.device_id+(data.has_password?' · Senha salva.':' · Sem senha salva.')}
async function load(){try{const response=await fetch('/api/homeassistant/config',{cache:'no-store',signal:AbortSignal.timeout(8000)});if(!response.ok)throw new Error('Não foi possível ler a configuração.');show(await response.json());el('save').disabled=false}catch(error){feedback(error.message,true)}}
el('ha-form').addEventListener('submit',async event=>{event.preventDefault();if(!el('ha-form').reportValidity())return;if(el('enabled').checked&&!el('host').value.trim()){feedback('Informe o endereço do broker.',true);return}el('save').disabled=true;try{const body=new URLSearchParams({enabled:el('enabled').checked?'1':'0',host:el('host').value.trim(),port:el('port').value,user:el('user').value,password:el('password').value,clear_password:el('clear-password').checked?'1':'0',interval:el('interval').value});const response=await fetch('/api/homeassistant/config',{method:'POST',body,headers:{'Content-Type':'application/x-www-form-urlencoded'},signal:AbortSignal.timeout(8000)});if(!response.ok)throw new Error(await response.text());show(await response.json());feedback('Configuração salva. O ESP32 tentará conectar ao broker; atualize a página para consultar o estado.')}catch(error){feedback(error.message,true)}finally{el('save').disabled=false}});
load();
</script></html>
)hapage";
#endif
