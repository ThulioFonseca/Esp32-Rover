#pragma once

// ets_printf() é uma função ROM do ESP32 que escreve diretamente no UART0
// via registros de hardware — sem mutex, sem FreeRTOS, ISR-safe.
// Substitui Serial.println() que usa xSemaphoreTake(portMAX_DELAY) internamente
// e crasha quando chamado com scheduler suspenso (AsyncTCP callbacks, flash writes).
extern "C" int ets_printf(const char *fmt, ...);
