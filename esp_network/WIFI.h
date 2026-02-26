#include "config.h"

String id(){
    //TODO make it from MAC address
}

void start_AP_mode(){

}
void stat_client_mode(){

}

void init_wifi(bool mode){
    if (mode == WIFI_MODE_CLIENT){
        stat_client_mode();
    }
    else if (mode == WIFI_MODE_AP){
        start_AP_mode();
    }

}