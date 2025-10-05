#ifndef WIFISYNC_H
#define WIFISYNC_H

void syncData();

void handleWiFi(unsigned long currentTime, unsigned long &lastWiFiRetryTime);

#endif