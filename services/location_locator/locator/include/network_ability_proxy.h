/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NETWORK_ABILITY_PROXY_H
#define NETWORK_ABILITY_PROXY_H

#include "iremote_object.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"

#include "network_ability_skeleton.h"

namespace OHOS {
namespace Location {
class NetworkAbilityProxy : public IRemoteProxy<INetworkAbility> {
public:
    explicit NetworkAbilityProxy(const sptr<IRemoteObject> &impl);
    ~NetworkAbilityProxy() = default;
    void SendLocationRequest(uint64_t interval, WorkRecord &workrecord) override;
    void SetEnable(bool state) override;
    void SelfRequest(bool state) override;
    bool EnableMock(const LocationMockConfig& config) override;
    bool DisableMock(const LocationMockConfig& config) override;
    bool SetMocked(const LocationMockConfig& config, const std::vector<std::shared_ptr<Location>> &location) override;
private:
    static inline BrokerDelegator<NetworkAbilityProxy> delegator_;
};
} // namespace Location
} // namespace OHOS
#endif // NETWORK_ABILITY_PROXY_H