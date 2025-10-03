
# Tank Robot Controller

Sistema de controle para robô tank com esteiras usando ESP32 e controle remoto iBUS.

## 🤖 Características

- Controle diferencial de esteiras via iBUS (10 canais)
- Compensação automática de deadband dos ESCs
- Sistema de timeout para segurança
- Mixing tank otimizado (throttle + steering)
- Arquitetura modular para expansão

## 🔧 Hardware Necessário

- **ESP32 Dev Board**
- **2x ESCs** compatíveis com PWM (1000-2000µs)
- **Controle remoto iBUS** (FlySky FS-i6, FS-i6X, etc.)
- **Receptor iBUS**
- **2x Motores DC** ou **Motores com redução**

## 📋 Conexões

```
ESP32          Componente
GPIO 19   →    ESC Esquerdo (sinal PWM)
GPIO 18   →    ESC Direito (sinal PWM)
GPIO 16   →    Receptor iBUS (TX)
5V        →    Alimentação receptor
GND       →    GND comum
```

## 🚀 Como Usar

### 1. Configuração do Ambiente
```bash
# Instalar PlatformIO
pip install platformio

# Clonar projeto
git clone <seu-repo>
cd tank-robot

# Build e upload
pio run -t upload

# Monitor serial
pio device monitor
```

### 2. Configuração do Controle
- **Canal 1 (CH_THROTTLE)**: Movimento frente/trás
- **Canal 2 (CH_STEERING)**: Direção esquerda/direita
- **Canais 3-10**: Disponíveis para expansão

### 3. Calibração dos ESCs
1. Ligue o ESP32
2. Sistema fará armamento automático (1.5s)
3. ESCs devem emitir bips de confirmação
4. Pronto para uso!

## ⚙️ Configurações

### Ajustar Deadband dos ESCs
Em `config/config.h`:
```cpp
constexpr int ESC_DEAD_LOW = 1482;   // Ajustar conforme seu ESC
constexpr int ESC_DEAD_HIGH = 1582;  // Ajustar conforme seu ESC
```

### Modificar Pinos
Em `config/pins.h`:
```cpp
constexpr uint8_t ESC_LEFT = 19;     // Pino ESC esquerdo
constexpr uint8_t ESC_RIGHT = 18;    // Pino ESC direito
```

### Timeout de Segurança
Em `config/config.h`:
```cpp
constexpr unsigned long IBUS_TIMEOUT_MS = 400; // Timeout em ms
```

## 📊 Output Serial Típico

```
=== Inicializando TankController ===
ChannelManager inicializado
MotorController inicializado
Iniciando sequência de armamento...
Sequência de armamento concluída.
*** ESTADO: ARMADO ***
=== Sistema Inicializado com Sucesso ===
CH: T:1500 S:1450 | nT:0.000 nS:-0.250 | MOT: L:-0.250 R:0.250 | PWM: L:1350 R:1750
```

## 🔍 Troubleshooting

### Robô não responde
- ✅ Verificar conexões iBUS
- ✅ Confirmar bind do receptor
- ✅ Verificar alimentação dos ESCs
- ✅ Monitor serial para debug

### ESCs não armam
- ✅ Verificar sequência de armamento
- ✅ Confirmar calibração dos ESCs
- ✅ Verificar alimentação (5V para receptor)

### Movimento irregular
- ✅ Ajustar deadband em `config.h`
- ✅ Verificar calibração do controle
- ✅ Confirmar range PWM dos ESCs

### Timeout constante
```
TIMEOUT: Sinal iBUS perdido - Motores em neutro
```
- ✅ Verificar bind receptor/transmissor
- ✅ Conferir pino de conexão iBUS
- ✅ Verificar alimentação do receptor

## 🔧 Desenvolvimento

### Estrutura do Código
```
src/
├── main.cpp              # Arquivo principal
├── config/               # Configurações
├── controllers/          # Lógica de controle
├── communication/        # Comunicação iBUS
├── types/               # Estruturas de dados
├── utils/               # Funções auxiliares
└── debug/               # Sistema de debug
```

### Adicionar Sensor
1. Definir em `types/types.h`
2. Criar classe em `sensors/`
3. Integrar em `TankController`

### Comandos Úteis
```bash
# Build apenas
pio run

# Upload + Monitor
pio run -t upload && pio device monitor

# Limpar build
pio run -t clean
```

## 📝 TODO

- [ ] Implementar leitura de bateria
- [ ] Adicionar telemetria WiFi
- [ ] Sistema de modos (manual/auto)
- [ ] Controle PID para direção
- [ ] Interface web para configuração

## 📄 Licença

Projeto pessoal - Use como quiser! 🚀

---

**Nota**: Este é um projeto de hobby. Use por sua conta e risco. Sempre teste em ambiente seguro!
```

## Características dos Arquivos:

### **`.gitignore`:**
- ✅ Ignora arquivos do PlatformIO
- ✅ Configurações pessoais do VSCode
- ✅ Arquivos temporários e logs
- ✅ Backups automáticos

### **`README.md`:**
- ✅ Linguagem simples e direta
- ✅ Instruções práticas de uso
- ✅ Troubleshooting comum
- ✅ Configurações essenciais
- ✅ Estrutura clara e navegável
- ✅ Emojis para facilitar leitura

Perfeito para um projeto pessoal! 🎯