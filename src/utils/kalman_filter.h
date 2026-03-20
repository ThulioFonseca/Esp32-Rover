#ifndef KALMAN_FILTER_H
#define KALMAN_FILTER_H

/**
 * Filtro de Kalman discreto para sinal escalar (1D).
 *
 * Modelo de processo:  x[k] = x[k-1] + w   (w ~ N(0, Q))
 * Modelo de medição:   z[k] = x[k]   + v   (v ~ N(0, R))
 *
 * Parâmetros de ajuste:
 *   processNoise     — pequeno = assume que o estado muda devagar
 *                      grande  = confia menos no modelo, reage mais rápido à medição
 *   measurementNoise — variância do ruído do sensor; maior = suaviza mais
 *
 * Exemplos de uso neste projeto:
 *   Compass heading: KalmanFilter1D(0.001f, 0.5f)  — heading muda devagar, HMC5883L é ruidoso
 */
class KalmanFilter1D {
public:
    KalmanFilter1D(float processNoise, float measurementNoise, float initialError = 1.0f)
        : kq(processNoise), kr(measurementNoise), kp(initialError),
          kx(0.0f), kready(false) {}

    // Incorpora nova medição e retorna o estado estimado filtrado.
    float update(float z) {
        if (!kready) {
            kx     = z;
            kready = true;
            return kx;
        }
        kp += kq;                       // predição: covariância cresce com a incerteza do processo
        float gain = kp / (kp + kr);   // ganho de Kalman (0=ignora medição, 1=ignora predição)
        kx    += gain * (z - kx);      // correção pelo resíduo
        kp    *= (1.0f - gain);        // covariância pós-atualização
        return kx;
    }

    float get()   const { return kx; }     // estado estimado atual (sem nova medição)
    bool  ready() const { return kready; } // true após a primeira medição
    void  reset()       { kready = false; kp = 1.0f; } // reinicia como se nunca tivesse recebido dados

private:
    float kq;      // ruído do processo (covariância) — fixo após construção
    float kr;      // ruído da medição  (covariância) — fixo após construção
    float kp;      // covariância do erro de estimativa
    float kx;      // estado estimado
    bool  kready;  // false antes da primeira medição
};

#endif
