#ifndef OTA_UPLOAD_PAGE_H
#define OTA_UPLOAD_PAGE_H

const char otaUploadForm[] = R"otaform(
<style>
input,button{font:inherit}button{padding:10px 18px;background:#ff9751;border:0;border-radius:8px;cursor:pointer}
button:disabled{opacity:.5;cursor:wait}progress{width:100%;height:25px;accent-color:#ff9751}
#ota-progress-box{margin:24px 0;padding:18px;background:#1b2330;border-radius:10px}
#ota-status{line-height:1.5}#ota-status.error{color:#ff9b9b}a{color:#ff9751}
</style>
<form id="ota-form" method="POST" action="/ota" enctype="multipart/form-data">
<input id="ota-file" type="file" name="firmware" accept=".bin" required>
<p><button id="ota-submit" type="submit">Enviar e atualizar</button></p></form>
<div id="ota-progress-box">
<label for="ota-progress">Atualização OTA · <strong id="ota-percent">0%</strong></label>
<progress id="ota-progress" max="100" value="0"></progress>
<p id="ota-status" role="status" aria-live="polite">Selecione o firmware para iniciar a gravação.</p>
<a id="ota-return" href="/info" hidden>Abrir painel após reiniciar</a>
</div>
<p>Aguarde a confirmação. O ESP32 reiniciará somente após validar o arquivo.</p>
<script>
(()=>{
const form=document.getElementById('ota-form'),file=document.getElementById('ota-file');
const button=document.getElementById('ota-submit'),box=document.getElementById('ota-progress-box');
const bar=document.getElementById('ota-progress'),status=document.getElementById('ota-status');
const back=document.getElementById('ota-return');
const percentage=document.getElementById('ota-percent');
let busy=false,percent=0;
const message=(text,error=false)=>{box.hidden=false;status.textContent=text;status.className=error?'error':''};
const failed=text=>{busy=false;button.disabled=false;file.disabled=false;bar.value=percent;percentage.textContent=percent+'% enviado';message(text,true)};
form.addEventListener('submit',event=>{
event.preventDefault();if(busy||!form.reportValidity())return;
const selected=file.files[0],limit=Number(document.getElementById('ota-limit').dataset.bytes);
percent=0;bar.value=0;percentage.textContent='0%';back.hidden=true;
if(!selected||!selected.size){message('Selecione um arquivo de firmware válido.',true);return}
if(selected.size>limit){message('O arquivo excede o limite de '+limit+' bytes da partição OTA.',true);return}
const body=new FormData(form),request=new XMLHttpRequest();
busy=true;button.disabled=true;file.disabled=true;message('Enviando e gravando firmware: 0%.');
request.open('POST',form.action);request.timeout=180000;
request.upload.onprogress=event=>{
if(event.lengthComputable){percent=Math.min(100,Math.floor(100*event.loaded/event.total));bar.value=percent;percentage.textContent=percent+'% enviado';message('Enviando e gravando firmware: '+percent+'%.')}
else{bar.removeAttribute('value');percentage.textContent='Enviando…';message('Enviando e gravando firmware…')}
};
request.upload.onload=()=>{percent=100;bar.removeAttribute('value');percentage.textContent='Validando…';message('Envio concluído. Aguardando validação do firmware pelo ESP32…')};
request.onload=()=>{
if(request.status!==200){failed(request.responseText||'O ESP32 recusou a atualização.');return}
bar.value=100;percentage.textContent='100% · Reiniciando';message('Firmware validado. ESP32 reiniciando… Aguarde cerca de 10 segundos para abrir o painel.');back.hidden=false;
};
request.onerror=()=>failed('Conexão perdida. Confira o dispositivo antes de tentar novamente.');
request.ontimeout=()=>failed('Tempo de espera esgotado. Confira o dispositivo antes de tentar novamente.');
request.onabort=()=>failed('Envio interrompido. Confira o dispositivo antes de tentar novamente.');
try{request.send(body)}catch(error){failed('Não foi possível iniciar o envio. Confira a conexão.')}
});
})();
</script>
)otaform";

#endif
