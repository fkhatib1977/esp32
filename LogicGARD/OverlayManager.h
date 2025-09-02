#pragma once

#include <memory>
#include "Types.h"
#include "MessageConsumer.h"
#include "SecureHttpClient.h"
#include "TemperatureMessage.h"
#include "MessageDispatcher.h"

class OverlayManager : public MessageConsumer {
public:
OverlayManager(const OverlayConfig& config, std::shared_ptr<SecureHttpClient> client);

  void begin(MessageDispatcher& dispatcher);
  int flag;

protected:
  void process(const TemperatureMessage& msg) override;
  
private:
  OverlayConfig config;
  std::shared_ptr<SecureHttpClient> client;

  int resolveIdentity(String indicator);
};