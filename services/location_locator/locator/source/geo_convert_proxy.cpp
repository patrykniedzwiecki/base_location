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

#include "geo_convert_proxy.h"
#include "ipc_skeleton.h"
#include "location_log.h"

namespace OHOS {
namespace Location {
GeoConvertProxy::GeoConvertProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IGeoConvert>(impl)
{
}

int GeoConvertProxy::IsGeoConvertAvailable(MessageParcel &reply)
{
    return SendSimpleMsg(IS_AVAILABLE, reply);
}

int GeoConvertProxy::GetAddressByCoordinate(MessageParcel &data, MessageParcel &reply)
{
    int error = REPLY_CODE_EXCEPTION;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LBSLOGE(GEO_CONVERT, "write interfaceToken fail!");
        return error;
    }
    error = SendMsgWithDataReply(GET_FROM_COORDINATE, data, reply);
    LBSLOGI(GEO_CONVERT, "GetAddressByCoordinate result from server.");
    return error;
}

int GeoConvertProxy::GetAddressByLocationName(MessageParcel &data, MessageParcel &reply)
{
    int error = REPLY_CODE_EXCEPTION;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LBSLOGE(GEO_CONVERT, "write interfaceToken fail!");
        return error;
    }
    error = SendMsgWithDataReply(GET_FROM_LOCATION_NAME_BY_BOUNDARY, data, reply);
    LBSLOGI(GEO_CONVERT, "GetAddressByLocationName result from server.");
    return error;
}

int GeoConvertProxy::SendSimpleMsg(const int msgId, MessageParcel& reply)
{
    int error = REPLY_CODE_EXCEPTION;
    MessageParcel data;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LBSLOGE(GEO_CONVERT, "write interfaceToken fail!");
        return error;
    }
    error = SendMsgWithDataReply(msgId, data, reply);
    return error;
}

int GeoConvertProxy::SendMsgWithDataReply(const int msgId, MessageParcel& data, MessageParcel& reply)
{
    int error = REPLY_CODE_EXCEPTION;
    MessageOption option;
    sptr<IRemoteObject> remote = Remote();
    if (remote == nullptr) {
        LBSLOGE(GEO_CONVERT, "SendMsgWithDataReply remote is null");
        return REPLY_CODE_EXCEPTION;
    }
    error = remote->SendRequest(msgId, data, reply, option);
    LBSLOGD(GEO_CONVERT, "Proxy::SendMsgWithDataReply result from server.");
    return error;
}

bool GeoConvertProxy::SendSimpleMsgAndParseResult(const int msgId)
{
    bool result = false;
    MessageParcel reply;
    int error = SendSimpleMsg(msgId, reply);
    if (error == NO_ERROR) {
        result = reply.ReadBool();
    }
    return result;
}

bool GeoConvertProxy::EnableReverseGeocodingMock()
{
    return SendSimpleMsgAndParseResult(ENABLE_REVERSE_GEOCODE_MOCK);
}

bool GeoConvertProxy::DisableReverseGeocodingMock()
{
    return SendSimpleMsgAndParseResult(DISABLE_REVERSE_GEOCODE_MOCK);
}

bool GeoConvertProxy::SetReverseGeocodingMockInfo(std::vector<std::shared_ptr<GeocodingMockInfo>>& mockInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        LBSLOGE(GEO_CONVERT, "write interfaceToken fail!");
        return false;
    }
    data.WriteInt32(mockInfo.size());
    for (size_t i = 0; i < mockInfo.size(); i++) {
        mockInfo[i]->Marshalling(data);
    }
    int error = SendMsgWithDataReply(SET_REVERSE_GEOCODE_MOCKINFO, data, reply);
    bool result = false;
    if (error == NO_ERROR) {
        result = reply.ReadBool();
    }
    return result;
}
} // namespace Location
} // namespace OHOS