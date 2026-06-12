/**
 * pressure_app.h
 * Üst-seviye uygulama: tüm modülleri başlatır, ana loop'u koşturur.
 * main.c sadece bu iki fonksiyonu çağırır.
 */
#ifndef PRESSURE_APP_H
#define PRESSURE_APP_H

#ifdef __cplusplus
extern "C" {
#endif

void pressure_app_init(void);
void pressure_app_loop(void);   /* while(1) içinden sürekli çağrılır */

#ifdef __cplusplus
}
#endif
#endif
