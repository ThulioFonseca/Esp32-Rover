# Tank Robot Controller - Contexto para Agentes IA

## Visão Geral do Projeto

Este é um sistema de controle para robô tank (esteiras) baseado em ESP32 que recebe comandos via protocolo iBUS e controla dois ESCs (Electronic Speed Controllers) para movimento diferencial das esteiras.

### Características Principais
- **Plataforma**: ESP32
- **Comunicação**: iBUS (10 canais)
- **Controle**: Tank diferencial (throttle + steering)
- **ESCs**: 2 unidades com deadband compensation
- **Arquitetura**: Modular orientada a objetos

## Estrutura do Projeto

```
src/
├── main.cpp                      # Ponto de entrada principal
├── config/
│   ├── config.h                 # Configurações do sistema (timeouts, PWM, etc.)
│   └── pins.h                   # Mapeamento de pinos do ESP32
├── types/
│   ├── types.h                  # Estruturas de dados e enums
│   └── types.cpp                # Implementação dos construtores
├── utils/
│   ├── utils.h                  # Funções utilitárias (matemática, conversões)
│   └── utils.cpp                # Implementação das funções utilitárias
├── controllers/
│   ├── tank_controller.h        # Controlador principal (coordenador)
│   ├── tank_controller.cpp      # Implementação do controlador principal
│   ├── motor_controller.h       # Controlador específico dos motores/ESCs
│   └── motor_controller.cpp     # Implementação do controle de motores
├── communication/
│   ├── channel_manager.h        # Gerenciamento de comunicação iBUS
│   └── channel_manager.cpp      # Implementação do gerenciamento iBUS
└── debug/
    ├── debug_manager.h          # Sistema de debug e logging
    └── debug_manager.cpp        # Implementação do sistema de debug
```

## Fluxo de Dados Principal

```
iBUS Radio → ChannelManager → TankController → MotorController → ESCs
                    ↓              ↓               ↓
                Validate       Process         Convert PWM
                Normalize      Mixing          Deadband Skip
                Timeout        Clamp           Output
```

## Configurações Críticas

### PWM e ESCs
```cpp
PWM_MIN = 1000          // Mínimo PWM padrão
PWM_MAX = 2000          // Máximo PWM padrão  
PWM_MID = 1500          // Centro neutro
ESC_DEAD_LOW = 1482     // Início deadband ESC
ESC_DEAD_HIGH = 1582    // Fim deadband ESC
ESC_NEUTRAL = 1532      // Centro real deadband
```

### Timing
```cpp
CONTROL_INTERVAL_MS = 20     // Loop principal (50Hz)
DEBUG_INTERVAL_MS = 100      // Debug output (10Hz)
IBUS_TIMEOUT_MS = 400        // Timeout comunicação
ARMING_TIME_MS = 1500        // Tempo armamento ESCs
```

## Classes Principais

### TankController
- **Responsabilidade**: Coordenação geral do sistema
- **Estados**: INITIALIZING, ARMING, ARMED, TIMEOUT, ERROR
- **Métodos importantes**:
  - `initialize()`: Setup completo do sistema
  - `update()`: Loop principal não-bloqueante
  - `setDebugMode(bool)`: Controle de debug

### ChannelManager  
- **Responsabilidade**: Comunicação iBUS e validação de dados
- **Funcionalidades**:
  - Leitura de canais iBUS
  - Normalização de valores (-1.0 a 1.0)
  - Detecção de timeout
  - Validação de dados

### MotorController
- **Responsabilidade**: Controle dos ESCs e mixing diferencial
- **Funcionalidades**:
  - Tank mixing: `left = throttle + steering`
  - Normalização de comandos (evita saturação)
  - Compensação de deadband
  - Sequência de armamento

### DebugManager
- **Responsabilidade**: Output de debug controlado
- **Funcionalidades**:
  - Debug com timing controlado
  - Estados do sistema
  - Dados de canais e motores

## Algoritmos Importantes

### Normalização de Stick
```cpp
// Converte PWM (1000-2000) para float (-1.0 a 1.0)
// Aplica deadzone para evitar jitter
// Considera centro real em 1500µs
```

### Tank Mixing
```cpp
left_motor = throttle + steering
right_motor = throttle - steering
// Normaliza se algum exceder ±1.0
```

### Compensação Deadband
```cpp
// ESCs possuem zona morta 1482-1582µs
// Algoritmo pula essa região mapeando para:
// Negativo: 1000-1481µs  
// Positivo: 1583-2000µs
```

## Pontos de Extensão

### Sensores (Preparado)
```cpp
namespace Sensors {
  struct SensorData {
    float batteryVoltage;
    float temperature; 
    float gyroZ;
    // ... adicionar novos sensores aqui
  };
}
```

### Canais Auxiliares
```cpp
// ChannelManager já lê todos os 10 canais
// aux[0-7] disponíveis em ChannelData
// Implementar processamento específico
```

### Telemetria
```cpp
namespace Telemetry {
  // Estrutura preparada para WiFi/Bluetooth
  // Envio de dados de estado, sensores, etc.
}
```

## Convenções de Código

### Nomenclatura
- **Classes**: PascalCase (`TankController`)
- **Métodos/Variáveis**: camelCase (`updateSystem`)
- **Constantes**: UPPER_SNAKE_CASE (`PWM_MIN`)
- **Namespaces**: PascalCase (`Config`, `Utils`)

### Organização
- Headers sempre com include guards
- Implementações em arquivos .cpp separados
- Configurações centralizadas em `config/`
- Tipos compartilhados em `types/`

### Performance
- Use `yield()` em loops longos
- Debug controlado por timing
- Evite `delay()` em código principal
- Prefira `millis()` para timing

## Padrões Utilizados

1. **Dependency Injection**: Controllers recebem dependências
2. **State Machine**: Estados bem definidos com transições
3. **Facade Pattern**: TankController esconde complexidade
4. **Strategy Pattern**: Diferentes behaviors por estado
5. **Observer Pattern**: Debug reativo a mudanças

## Limitações Conhecidas

1. **Watchdog**: ESP32 requer `yield()` periódico
2. **iBUS**: Apenas canais 0-1 processados no loop principal
3. **ESCs**: Deadband hardcoded para modelos específicos
4. **Memory**: Estruturas otimizadas para RAM limitada

## Modificações Comuns

### Adicionar Novo Sensor
1. Definir em `types/types.h` → `SensorData`
2. Criar classe em `sensors/`
3. Integrar em `TankController::update()`
4. Adicionar configurações em `config/`

### Modificar Timing
1. Alterar constantes em `config/config.h`
2. Verificar impacto em watchdog
3. Testar estabilidade do sistema

### Adicionar Modo de Operação
1. Estender enum `SystemState` em `types/types.h`
2. Adicionar case em `TankController::update()`
3. Implementar lógica específica

### Configurar ESCs Diferentes
1. Modificar valores deadband em `config/config.h`
2. Ajustar função `denormalizeToEsc()` se necessário
3. Testar sequência de armamento

## Dependências Externas

```cpp
#include <IBusBM.h>      // Comunicação iBUS
#include <ESP32Servo.h>  // Controle ESCs via PWM
#include <Arduino.h>     // Framework base
```

## Debugging

### Saída Serial Típica
```
CH: T:1500 S:1450 | nT:0.000 nS:-0.250 | MOT: L:-0.250 R:0.250 | PWM: L:1350 R:1750
*** ESTADO: ARMADO ***
TIMEOUT: Sinal iBUS perdido - Motores em neutro
```

### Pontos de Breakpoint Importantes
- `TankController::update()` - Loop principal
- `ChannelManager::updateChannelData()` - Recepção dados
- `MotorController::calculateMotorCommands()` - Mixing
- `Utils::denormalizeToEsc()` - Conversão PWM

## Performance Targets

- **Loop Principal**: 50Hz (20ms)
- **Debug Output**: 10Hz (100ms)  
- **CPU Load**: <50% para permitir expansão
- **RAM Usage**: <60% para estabilidade

## Segurança

1. **Timeout obrigatório**: Sistema vai para neutro em 400ms
2. **Armamento necessário**: ESCs devem ser inicializados
3. **Validação entrada**: Dados iBUS verificados antes uso
4. **Estados bem definidos**: Transições controladas
5. **Watchdog compliance**: `yield()` previne reset

---

**Nota para Agentes IA**: Este sistema está otimizado para robustez e extensibilidade. Sempre considere impacto no watchdog ao fazer modificações no loop principal. Priorize segurança sobre performance - é melhor um robô lento que um robô descontrolado.