#include "Arduino.h"
#include "src/Sonde.h"

boolean ativar_deep_sleep = 0;
uint32_t time_rx_off = 0;
uint8_t fora_de_hora = 0;
boolean perda_sinal = 0;
int rx_status_anterior = 0;
// DEEP SLEEP   #####################################################

void deep_sleep(int rx_status) {

//   if(sonde.config.deep_sleep>0) { // se está configurada a funcao deep sleep

    #define uS_TO_S_FACTOR 1000000LL /* Conversion factor for micro seconds to seconds */ 
    #define GMT -3
    int8_t hora_ligar = 8;
    int8_t min_ligar = 30;
    int8_t hora_desligar = 9;
    int8_t min_desligar = 30;   
    uint16_t seg_restante;
    int8_t hora_restante;
    int8_t min_restante;
    
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      return;
    }
    int8_t hora_atual = timeinfo.tm_hour + GMT;
    int8_t min_atual = timeinfo.tm_min;
    if(hora_atual>12) hora_atual-=12;
    else if(hora_atual<0) hora_atual+=12;

    Serial.printf("\nHORA ATUAL: %dh%dm%ds", hora_atual,min_atual,timeinfo.tm_sec);
    hora_restante = hora_ligar - hora_atual;
    Serial.printf("\nHORA restante: %d", hora_restante);
    min_restante = min_ligar - min_atual - 1;
    Serial.printf("\nMINUTO restante: %d", min_restante);
    if(hora_restante<0) {
      hora_restante += 12;
    }
    if(min_restante<0 && hora_restante==0) {
      min_restante += 59;
      hora_restante +=11;
    }
    else if(min_restante<0 && hora_restante>0) {
      min_restante += 59;
      hora_restante--;
    }
    seg_restante = hora_restante*3600 + min_restante*60 + 60-timeinfo.tm_sec;
    Serial.printf("\nTEMPO PARA RELIGAR: %dh%dm%ds", hora_restante, min_restante, 60-timeinfo.tm_sec);
     //res = 0x8001 ainda não recebeu sinal / 0xFF00 recebendo sinal da sonda / 0xFF01 perdeu contato com a sonda e volta para o 8001 tela só com o ip / 0xFF02 crc error
    if(ativar_deep_sleep) Serial.printf("\nTEMPO EXTRA: %lu", (time_rx_off - millis()) / 1000);
    else Serial.printf("\nTEMPO EXTRA: LIGADO DIRETO");
    Serial.printf("\nrx_status: %i - rx_status_anterior: %i", rx_status, rx_status_anterior);
    if ((rx_status == 255 || rx_status == 63) && (rx_status != rx_status_anterior)) // se a sonda está recebendo ou iniciou o recebimento, desliga o timer
      {
        rx_status_anterior = rx_status;
        ativar_deep_sleep = 0;
        time_rx_off = 0xFFFF;
        Serial.printf("\nSONDA RECEBENDO SINAL");
      }
    if ((rx_status == 128 || rx_status == 63 ) && (rx_status != rx_status_anterior))  { // perdeu contato com a sonda
      rx_status_anterior = rx_status;
      ativar_deep_sleep = 1;
      perda_sinal = 1;
      time_rx_off = millis() + 5 * 60 * 1000; // o tempo para desligar é a hora atual mais os minutos
      Serial.printf("\nPERDEU CONTATO COM A SONDA, DESLIGANDO EM 5 min");
    }
    if ((rx_status == 128) && (rx_status != rx_status_anterior) && (rx_status_anterior == 0))  { //ainda nao recebeu sinal da sonda 
      rx_status_anterior = rx_status;
      ativar_deep_sleep = 1; 
      perda_sinal = 1;    
      time_rx_off = millis() + 1 * 60 * 1000; // o tempo para desligar é de 10 min se ainda não recebeu sinal nenhum  
      Serial.printf("\nAINDA NENHUM SINAL RECEBIDO, DESLIGANDO EM 1 MIN ");
    } 
    if ((ativar_deep_sleep==1) && (fora_de_hora==0) && (((hora_atual>hora_desligar) || (hora_atual==hora_desligar && min_atual>=min_desligar)) || ((hora_ligar>hora_atual) || (hora_ligar==hora_atual && min_ligar>min_atual))))  
      {    
      fora_de_hora = 1;
      time_rx_off = millis() + 1 * 60 * 1000; // se for fora de hora fica somente 1 min ligado  
      Serial.printf("\nFORA DE HORA, DESLIGANDO EM 1 MIN");
      }
    if((ativar_deep_sleep==1) && (fora_de_hora==0) && (((hora_atual>hora_ligar) || (hora_atual==hora_ligar && min_atual>=min_ligar)) && ((hora_desligar>hora_atual) || (hora_desligar==hora_atual && min_desligar>=min_atual))))
    {
      fora_de_hora = 2;
      time_rx_off = millis() + 1 * 60 * 1000; // se for na hora fica somente 1 min ligado
      Serial.printf("\nDENTRO DA HORA DE RECEPCAO,  = 2");
    }
    if ((millis() > time_rx_off) && (ativar_deep_sleep))  // se a sonda estpa ligada não entra na contagem pra desligamento do rx
      {
      sonde.clearDisplay();
      Serial.flush();
      if(fora_de_hora==2) {
        Serial.printf("\nRELIGARÁ EM: %u segundos\n", 5 * 60);  
        esp_sleep_enable_timer_wakeup(5 * 60 * uS_TO_S_FACTOR); // se dentro do horario de espera, liga novamente em 5 min --- 30s lig / 5 min desl
      }      
      else {
        Serial.printf("\nDEEP_SLEEP: %llu MICRO_segundos\n", uint64_t (seg_restante) * uS_TO_S_FACTOR);
        esp_sleep_enable_timer_wakeup(uint64_t (seg_restante) * uS_TO_S_FACTOR);
      }
      esp_deep_sleep_start();
      }

    // Serial.printf("\nTYPE_DEEP_SLEEP: %u\n", sizeof(uint64_t (seg_restante)));
    // Serial.printf("\nTYPE seg_DEEP_SLEEP: %u\n", sizeof(seg_restante));
    Serial.printf("\nSegundos para religar: %u\n", seg_restante);

    // }
}
    


// DEEP SLEEP   #####################################################