# Monitoramento de Qualidade do Ar com ESP32

## Descrição
Este projeto utiliza uma ESP32 para monitorar dados ambientais, incluindo temperatura, umidade e qualidade do ar, com base em sensores DHT11 e MQ-135. Os dados capturados são enviados periodicamente para uma API HTTP para armazenamento e análise.

---

## Componentes Utilizados
- **Microcontrolador:** ESP32
- **Sensores:**
  - **DHT11:** Leitura de temperatura e umidade
  - **MQ-135:** Monitoramento da qualidade do ar (CO2, NH3, NOx)
- **Conexão Wi-Fi:** Para enviar os dados à API

---

## Formato dos Dados Enviados
Os dados enviados para a API estão no seguinte formato JSON:

```json
{
  "sensor_id": "esp32-001",
  "timestamp": "YYYY-MM-DDTHH:MM:SSZ",
  "data": {
    "temperature": 30,
    "humidity": 65,
    "gases": {
      "co2": 400,
      "nh3": 12,
      "nox": 20
    },
    "aqi": 85
  },
  "alerts": {
    "status": "ok",
    "messages": []
  }
}
```

# Funcionalidades

- Captura de dados ambientais usando sensores **DHT11** e **MQ-135**.
- Conexão ao **Wi-Fi** para comunicação com a API.
- Envio de dados para a API HTTP em intervalos de **30 minutos**.

---

# Configuração

## Conexão com o Wi-Fi
- Insira o nome da rede (**SSID**) e a senha no código.

## API Endpoint
- Substitua a **URL do servidor** na variável `serverName` com o endpoint da sua API.

## Sensor ID
- Atualize o campo `sensor_id` no JSON com o identificador do seu dispositivo.

---

# Como Usar

## Configuração do Ambiente
1. Certifique-se de que a placa **ESP32** está configurada corretamente na **IDE do Arduino**.
2. Instale as bibliotecas necessárias: **DHT** e **MQ135**.

## Upload do Código
1. Conecte a **ESP32** ao computador.
2. Faça o upload do código para a placa através da **Arduino IDE**.

## Monitoramento
- Utilize o **monitor serial** para visualizar os dados capturados.
- Verifique se os dados estão sendo enviados corretamente para a API.

---

# Considerações

- Este projeto é inicial e utiliza **HTTP** para enviar dados. Para maior segurança e desempenho, considere migrar para **HTTPS** ou **MQTT** em versões futuras.
- Certifique-se de que sua **API** está funcionando corretamente e configurada para receber os dados enviados.
