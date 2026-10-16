#ifndef BLUETOOTH_SERVICE_H
#define BLUETOOTH_SERVICE_H

#include <Arduino.h>
#include <string.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class BluetoothService {
private:
    struct Mensagem {
        char texto[256];
        uint32_t sessao;
        bool erro;
    };
    QueueHandle_t fila = nullptr;
    BLEServer* servidor = nullptr;
    BLECharacteristic* tx = nullptr;
    std::atomic<bool> conectado{false};
    std::atomic<bool> reiniciarAnuncio{false};
    std::atomic<bool> filaCheia{false};
    std::atomic<uint32_t> sessao{0};
    std::atomic<unsigned long> desconectadoEm{0};
    char recebendo[256] = {};
    size_t tamanho = 0;
    bool descartar = false;
    String respostaPendente;
    size_t posicaoTX = 0;
    unsigned long ultimoTX = 0;
    uint32_t sessaoResposta = 0;

    void receber(const String& dados) {
        // Uma linha pode chegar em varias escritas BLE de 20 bytes.
        for (size_t i = 0; i < dados.length(); ++i) {
            uint8_t c = dados[i];
            if (c == '\r') continue;
            if (c == '\n') {
                if (tamanho || descartar) {
                    Mensagem mensagem{};
                    mensagem.sessao = sessao.load();
                    mensagem.erro = descartar;
                    if (!descartar) memcpy(mensagem.texto, recebendo, tamanho);
                    if (xQueueSend(fila, &mensagem, 0) != pdTRUE) filaCheia.store(true);
                }
                tamanho = 0;
                descartar = false;
            } else if (c < 32 || c > 126 || tamanho >= 255) {
                descartar = true;
            } else if (!descartar) {
                recebendo[tamanho++] = c;
            }
        }
    }

    class ServidorCallbacks : public BLEServerCallbacks {
        BluetoothService& dono;
    public:
        explicit ServidorCallbacks(BluetoothService& servico) : dono(servico) {}
        void onConnect(BLEServer*) override {
            dono.sessao.fetch_add(1);
            dono.conectado.store(true);
            Serial.println(F("[BLE] Cliente conectado."));
        }
        void onDisconnect(BLEServer*) override {
            dono.conectado.store(false);
            dono.sessao.fetch_add(1);
            dono.tamanho = 0;
            dono.descartar = false;
            dono.filaCheia.store(false);
            dono.desconectadoEm.store(millis());
            dono.reiniciarAnuncio.store(true);
            Serial.println(F("[BLE] Cliente desconectado."));
        }
    };

    class RecepcaoCallbacks : public BLECharacteristicCallbacks {
        BluetoothService& dono;
    public:
        explicit RecepcaoCallbacks(BluetoothService& servico) : dono(servico) {}
        void onWrite(BLECharacteristic* caracteristica) override {
            dono.receber(caracteristica->getValue());
        }
    };

    void enviarParte() {
        if (!tx || respostaPendente.isEmpty()) return;
        if (!conectado.load() || sessao.load() != sessaoResposta) {
            respostaPendente = "";
            return;
        }
        size_t restante = respostaPendente.length() - posicaoTX;
        size_t quantidade = restante > 20 ? 20 : restante;
        // Evita separar um caractere UTF-8 entre duas notificacoes.
        while (quantidade > 0 && posicaoTX + quantidade < respostaPendente.length() &&
               (static_cast<uint8_t>(respostaPendente[posicaoTX + quantidade]) & 0xC0) == 0x80) {
            --quantidade;
        }
        if (!quantidade) quantidade = restante > 20 ? 20 : restante;
        tx->setValue(reinterpret_cast<const uint8_t*>(respostaPendente.c_str() + posicaoTX), quantidade);
        tx->notify();
        posicaoTX += quantidade;
        ultimoTX = millis();
        if (posicaoTX >= respostaPendente.length()) respostaPendente = "";
    }

public:
    bool estaAtivo() const { return servidor != nullptr && tx != nullptr; }
    bool estaConectado() const { return conectado.load(); }
    unsigned int comandosPendentes() const { return fila ? uxQueueMessagesWaiting(fila) : 0; }
    bool begin() {
        if (servidor) return true;
        fila = xQueueCreate(4, sizeof(Mensagem));
        if (!fila) {
            Serial.println(F("[BLE] Sem memoria para fila de comandos."));
            return false;
        }
        BLEDevice::init("M5NanoC6");
        BLEDevice::setMTU(247);
        servidor = BLEDevice::createServer();
        servidor->setCallbacks(new ServidorCallbacks(*this));
        BLEService* servico = servidor->createService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
        tx = servico->createCharacteristic("6E400003-B5A3-F393-E0A9-E50E24DCCA9E",
                                           BLECharacteristic::PROPERTY_NOTIFY);
        // NimBLE do ESP32-C6 adiciona automaticamente o descritor de notificacao.
        BLECharacteristic* rx = servico->createCharacteristic("6E400002-B5A3-F393-E0A9-E50E24DCCA9E",
            BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
        rx->setCallbacks(new RecepcaoCallbacks(*this));
        servico->start();
        BLEAdvertising* anuncio = servidor->getAdvertising();
        anuncio->addServiceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
        anuncio->setScanResponse(true);
        anuncio->start();
        Serial.println(F("[BLE] M5NanoC6 ativo. Comandos terminados em LF (\\n)."));
        return true;
    }

    void atualizar() {
        if (!servidor) return;
        if (reiniciarAnuncio.load() && !conectado.load() &&
            millis() - desconectadoEm.load() >= 300UL) {
            servidor->startAdvertising();
            reiniciarAnuncio.store(false);
        }
        if (millis() - ultimoTX >= 20UL) enviarParte();
    }

    bool checarMensagem(String& comando) {
        if (!fila || !conectado.load() || !respostaPendente.isEmpty()) return false;
        Mensagem mensagem;
        while (xQueueReceive(fila, &mensagem, 0) == pdTRUE) {
            if (mensagem.sessao != sessao.load()) continue;
            sessaoResposta = mensagem.sessao;
            if (mensagem.erro) {
                responder("Erro: use texto ASCII de ate 255 bytes seguido de LF.\n");
                return false;
            }
            comando = mensagem.texto;
            return true;
        }
        if (filaCheia.exchange(false)) {
            sessaoResposta = sessao.load();
            responder("Erro: fila BLE cheia. Aguarde a resposta antes do proximo comando.\n");
        }
        return false;
    }

    void responder(const String& resposta) {
        if (!conectado.load() || sessao.load() != sessaoResposta) return;
        if (respostaPendente.isEmpty()) posicaoTX = 0;
        respostaPendente += resposta;
        // Primeiro trecho imediato permite confirmar REBOOT antes de reiniciar.
        enviarParte();
    }
};

extern BluetoothService bluetooth;

#endif
