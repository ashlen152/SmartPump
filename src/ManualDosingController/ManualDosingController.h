#ifndef MANUAL_DOSING_CONTROLLER_H
#define MANUAL_DOSING_CONTROLLER_H

void beginManualDosingController(bool isInManualBegin);

void startManualDosingController(bool isInManualStart);

void progressManualDosingController(bool isInManualProgress);

void completeManualDosingController(bool isInManualComplete);

#endif