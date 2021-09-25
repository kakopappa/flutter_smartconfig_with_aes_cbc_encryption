
 
#ifndef SMARTCONFIGPROV_H_
#define SMARTCONFIGPROV_H_

#include "ESP8266WiFi.h"  
#include "smartconfig.h"
#include "Base64.h"

class SmartConfigProv
{  
public: 
  bool beginSmartConfig();
  bool stopSmartConfig();
  bool smartConfigDone();

  static bool _smartConfigDone;
protected:
  static bool _smartConfigStarted;
  static void _smartConfigCallback(uint32_t status, void* result); 
  
  static constexpr  uint8_t cipher_key[16] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
  static constexpr  uint8_t cipher_iv[16] =  {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
};
 

bool SmartConfigProv::_smartConfigStarted = false;
bool SmartConfigProv::_smartConfigDone = false;

/**
 * Start SmartConfig
 */
bool SmartConfigProv::beginSmartConfig() {
    int DEBUG = 1;
    
    if (_smartConfigStarted) {
        return false;
    }

    if(!WiFi.enableSTA(true)) {
        // enable STA failed
        return false;
    }
    
    if(smartconfig_start(reinterpret_cast<sc_callback_t>(&SmartConfigProv::_smartConfigCallback), DEBUG)) {
        _smartConfigStarted = true;
        _smartConfigDone = false;
        return true;
    }
    
    return false; 
}

/**
 *  Stop SmartConfig
 */
bool SmartConfigProv::stopSmartConfig() {
    if (!_smartConfigStarted) {
        return true;
    }

    if (smartconfig_stop()) {
        _smartConfigStarted = false;
        return true;
    }

    return false;
}

/**
 * Query SmartConfig status, to decide when stop config
 * @return smartConfig Done
 */
bool SmartConfigProv::smartConfigDone() {
    if (!_smartConfigStarted) {
        return false;
    }

    return _smartConfigDone;
}

/**
 * _smartConfigCallback
 * @param st
 * @param result
 */
void SmartConfigProv::_smartConfigCallback(uint32_t st, void* result) {
    sc_status status = (sc_status) st;
    
    if(status == SC_STATUS_LINK) {
        station_config* sta_conf = reinterpret_cast<station_config*>(result);
        
        int encoded_len = strlen((char*)sta_conf->password);
        char *encoded_data = (char*)&sta_conf->password[0];
    
        // base64 decode
        int len = base64_dec_len(encoded_data, encoded_len);
        uint8_t data[ len ];
        base64_decode((char *)data, encoded_data, encoded_len);

        // make a local copy of the key, iv
        uint8_t key[16], iv[16];
        memcpy(key, cipher_key, 16);
        memcpy(iv, cipher_iv, 16);

        // aes cbc decrypt
        int n_blocks = len / 16;      
        br_aes_big_cbcdec_keys decCtx;      
        br_aes_big_cbcdec_init(&decCtx, key, 16);
        br_aes_big_cbcdec_run( &decCtx, iv, data, n_blocks*16 ); 
      
        // PKCS#7 Padding
        uint8_t n_padding = data[n_blocks*16-1];
        len = n_blocks*16 - n_padding;

        int plain_data_len = len + 1;
        char plain_data[plain_data_len];
        memcpy(plain_data, data, len);
        plain_data[len] = '\0'; 

        // copy the decrypted password
        memcpy(sta_conf->password, plain_data, plain_data_len);

        Serial.println((char *)sta_conf->password);

        // connect to wifi 
        wifi_station_set_config(sta_conf);
        wifi_station_disconnect();
        wifi_station_connect();

        _smartConfigDone = true;
    } else if(status == SC_STATUS_LINK_OVER) {
//        if(result){
//            ip4_addr_t * ip = (ip4_addr_t *)result;
//            Serial.printf("Sender IP: " IPSTR, IP2STR(ip));
//        }
        
        WiFi.stopSmartConfig();
    }
}

 

#endif  
