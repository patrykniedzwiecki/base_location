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

#ifndef GNSS_ABILITY_SKELETON_H
#define GNSS_ABILITY_SKELETON_H

#include "message_option.h"
#include "message_parcel.h"
#include "iremote_object.h"
#include "iremote_stub.h"

#include "constant_definition.h"
#include "location_mock_config.h"
#include "subability_common.h"

namespace OHOS {
namespace Location {
class IGnssAbility : public ISubAbility {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"location.IGnssAbility");
    virtual void RefrashRequirements() = 0;
    virtual void RegisterGnssStatusCallback(const sptr<IRemoteObject>& callback, pid_t uid) = 0;
    virtual void UnregisterGnssStatusCallback(const sptr<IRemoteObject>& callback) = 0;
    virtual void RegisterNmeaMessageCallback(const sptr<IRemoteObject>& callback, pid_t uid) = 0;
    virtual void UnregisterNmeaMessageCallback(const sptr<IRemoteObject>& callback) = 0;
    virtual void RegisterCachedCallback(const std::unique_ptr<CachedGnssLocationsRequest>& request,
        const sptr<IRemoteObject>& callback) = 0;
    virtual void UnregisterCachedCallback(const sptr<IRemoteObject>& callback) = 0;

    virtual int GetCachedGnssLocationsSize() = 0;
    virtual int FlushCachedGnssLocations() = 0;
    virtual void SendCommand(std::unique_ptr<LocationCommand>& commands) = 0;
    virtual void AddFence(std::unique_ptr<GeofenceRequest>& request) = 0;
    virtual void RemoveFence(std::unique_ptr<GeofenceRequest>& request) = 0;
};

class GnssAbilityStub : public IRemoteStub<IGnssAbility> {
public:
    int32_t OnRemoteRequest(uint32_t code,
        MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    virtual void SendMessage(uint32_t code, MessageParcel &data, MessageParcel &reply) = 0;
};
} // namespace Location
} // namespace OHOS
#endif // GNSS_ABILITY_SKELETON_H
