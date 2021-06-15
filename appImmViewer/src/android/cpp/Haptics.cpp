#include "Haptics.h"

#include <VrApi_Input.h>

uint8_t waitingBuffer[]{255};
const ovrHapticBuffer waitingHaptics{0.0, 10, true, waitingBuffer};

uint8_t hoverBuffer[]{255, 255, 255, 255, 255};
const ovrHapticBuffer hoverHaptics{0.0, sizeof(hoverBuffer), true, hoverBuffer};

uint8_t scrollBuffer[]{100, 90, 80, 70, 60, 50, 40, 30, 20, 10};
const ovrHapticBuffer scrollHaptics{0.0, sizeof(scrollBuffer), true, scrollBuffer};

uint8_t selectBuffer[]{255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
const ovrHapticBuffer selectHaptics{0.0, sizeof(selectBuffer), true, selectBuffer};

uint8_t closeBuffer[]{10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
const ovrHapticBuffer closeHaptics{0.0, sizeof(closeBuffer), true, closeBuffer};

const std::unordered_map<std::string, const ovrHapticBuffer*> HapticEffects{
    {"waiting1", &waitingHaptics},
    {"hover1", &hoverHaptics},
    {"scroll1", &scrollHaptics},
    {"select1", &selectHaptics},
    {"close1", &closeHaptics}};

void HapticEffect(ImmCore::piLog * log, ovrMobile* ovr, const ovrDeviceID deviceID, const char* effectName, const int count, const int total) {
  if (ovr == nullptr || deviceID == ovrDeviceIdType_Invalid || effectName == nullptr) {
    return;
  }

  auto map = HapticEffects.find(effectName);
  if (map == HapticEffects.end()) {
    return;
  }

  ovrInputTrackedRemoteCapabilities cap;
  cap.Header.Type = ovrControllerType_TrackedRemote;
  cap.Header.DeviceID = deviceID;

  vrapi_GetInputDeviceCapabilities(ovr, &cap.Header);
  if (!(cap.ControllerCapabilities & ovrControllerCaps_HasBufferedHapticVibration)) {
    log->Printf(LT_WARNING, L"Device %i doesn't have BufferedHapticVibration", deviceID);
    return;
  }
  log->Printf(LT_DEBUG, L"Starting haptic %s on device %i", effectName, deviceID);
  ovrHapticBuffer hap = *map->second;

  if (hap.HapticBuffer == waitingBuffer)
  {
    size_t sampleCount = 10;
    uint8_t waitingBuffer[10];
    for (int sample = 0; sample < sampleCount; sample++)
    {
      float i = (float) sample / (float) sampleCount;
      float d = (count + i) / (0.333f * total);
      d = d - (int) d;
      waitingBuffer[sample] = (uint8_t) (255.0f * sin(3.14159f * d));
    }

    hap.HapticBuffer = waitingBuffer;
    hap.Terminated = count == total;
  }

  hap.BufferTime = vrapi_GetTimeInSeconds() + 0.010;
  vrapi_SetHapticVibrationBuffer(ovr, deviceID, &hap);
}
