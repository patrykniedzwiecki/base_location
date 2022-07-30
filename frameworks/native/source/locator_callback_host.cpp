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

#include "locator_callback_host.h"

#include "async_context.h"
#include "ipc_object_stub.h"
#include "js_native_api.h"
#include "js_native_api_types.h"
#include "message_option.h"
#include "message_parcel.h"
#include "napi/native_common.h"
#include "node_api.h"
#include "refbase.h"
#include "uv.h"

#include "common_utils.h"
#include "constant_definition.h"
#include "i_locator_callback.h"
#include "location.h"
#include "location_log.h"
#include "napi_util.h"
#include "location_async_context.h"

namespace OHOS {
namespace Location {
LocatorCallbackHost::LocatorCallbackHost()
{
    m_env = nullptr;
    m_handlerCb = nullptr;
    m_successHandlerCb = nullptr;
    m_failHandlerCb = nullptr;
    m_completeHandlerCb = nullptr;
    m_deferred = nullptr;
    m_fixNumber = 0;
    m_singleLocation = nullptr;
    InitLatch();
}

void LocatorCallbackHost::InitLatch()
{
    m_latch = new CountDownLatch();
    m_latch->SetCount(1);
}

LocatorCallbackHost::~LocatorCallbackHost()
{
    delete m_latch;
}

int LocatorCallbackHost::OnRemoteRequest(uint32_t code,
    MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    if (data.ReadInterfaceToken() != GetDescriptor()) {
        LBSLOGE(LOCATOR_CALLBACK, "invalid token.");
        return -1;
    }

    switch (code) {
        case RECEIVE_LOCATION_INFO_EVENT: {
            std::unique_ptr<Location> location = Location::Unmarshalling(data);
            LBSLOGI(LOCATOR_STANDARD, "CallbackSutb receive LOCATION_EVENT.");
            OnLocationReport(location);
            m_singleLocation = std::move(location);
            CountDown();
            break;
        }
        case RECEIVE_LOCATION_STATUS_EVENT: {
            int status = data.ReadInt32();
            LBSLOGI(LOCATOR_STANDARD, "CallbackSutb receive STATUS_EVENT. status:%{public}d", status);
            OnLocatingStatusChange(status);
            break;
        }
        case RECEIVE_ERROR_INFO_EVENT: {
            int errorCode = data.ReadInt32();
            LBSLOGI(LOCATOR_STANDARD, "CallbackSutb receive ERROR_EVENT. errorCode:%{public}d", errorCode);
            OnErrorReport(errorCode);
            break;
        }
        default: {
            IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            break;
        }
    }
    return 0;
}

void LocatorCallbackHost::DoSendWork(uv_loop_s*& loop, uv_work_t*& work)
{
    uv_queue_work(loop, work, [](uv_work_t* work) {},
        [](uv_work_t* work, int status) {
            if (work == nullptr) {
                return;
            }
            auto context = static_cast<LocationAsyncContext*>(work->data);
            if (context == nullptr) {
                delete work;
                return;
            }
            napi_handle_scope scope = nullptr;
            NAPI_CALL_RETURN_VOID(context->env, napi_open_handle_scope(context->env, &scope));
            if (scope == nullptr || context->loc == nullptr) {
                delete context;
                delete work;
                return;
            }
            napi_value jsEvent = nullptr;
            NAPI_CALL_RETURN_VOID(context->env, napi_create_object(context->env, &jsEvent));
            if (context->callback[1]) {
                SystemLocationToJs(context->env, context->loc, jsEvent);
            } else {
                LocationToJs(context->env, context->loc, jsEvent);
            }
            if (context->callback[0] != nullptr) {
                napi_value undefine = nullptr, handler = nullptr;
                NAPI_CALL_RETURN_VOID(context->env, napi_get_undefined(context->env, &undefine));
                NAPI_CALL_RETURN_VOID(context->env,
                    napi_get_reference_value(context->env, context->callback[0], &handler));
                if (napi_call_function(context->env, nullptr, handler, 1,
                    &jsEvent, &undefine) != napi_ok) {
                    LBSLOGE(LOCATOR_CALLBACK, "Report location failed");
                }
            } else if (context->deferred != nullptr) {
                if (jsEvent != nullptr) {
                    NAPI_CALL_RETURN_VOID(context->env,
                        napi_resolve_deferred(context->env, context->deferred, jsEvent));
                } else {
                    NAPI_CALL_RETURN_VOID(context->env,
                        napi_reject_deferred(context->env, context->deferred, jsEvent));
                }
            }
            NAPI_CALL_RETURN_VOID(context->env, napi_close_handle_scope(context->env, scope));
            delete context;
            delete work;
    });
}

void LocatorCallbackHost::DoSendErrorCode(uv_loop_s *&loop, uv_work_t *&work)
{
    uv_queue_work(loop, work, [](uv_work_t *work) {},
        [](uv_work_t *work, int status) {
            AsyncContext *context = nullptr;
            napi_handle_scope scope = nullptr;
            if (work == nullptr) {
                LBSLOGE(LOCATOR_CALLBACK, "work is nullptr");
                return;
            }
            context = static_cast<AsyncContext *>(work->data);
            if (context == nullptr) {
                LBSLOGE(LOCATOR_CALLBACK, "context is nullptr");
                delete work;
                return;
            }
            NAPI_CALL_RETURN_VOID(context->env, napi_open_handle_scope(context->env, &scope));
            if (scope == nullptr) {
                LBSLOGE(LOCATOR_CALLBACK, "scope is nullptr");
                delete context;
                delete work;
                return;
            }
            if (context->callback[FAIL_CALLBACK] != nullptr) {
                napi_value undefine;
                napi_value handler = nullptr;
                NAPI_CALL_RETURN_VOID(context->env, napi_get_undefined(context->env, &undefine));
                NAPI_CALL_RETURN_VOID(context->env,
                    napi_get_reference_value(context->env, context->callback[FAIL_CALLBACK], &handler));
                std::string msg = GetErrorMsgByCode(context->errCode);
                CreateFailCallBackParams(*context, msg, context->errCode);
                if (napi_call_function(context->env, nullptr, handler, RESULT_SIZE,
                    context->result, &undefine) != napi_ok) {
                    LBSLOGE(LOCATOR_CALLBACK, "Report system error failed");
                }
            }
            NAPI_CALL_RETURN_VOID(context->env, napi_close_handle_scope(context->env, scope));
            delete context;
            delete work;
    });
}

bool LocatorCallbackHost::InitContext(AsyncContext* context)
{
    if (context == nullptr) {
        LBSLOGE(LOCATOR_CALLBACK, "context == nullptr.");
        return false;
    }
    context->env = m_env;
    if (IsSystemGeoLocationApi()) {
        context->callback[SUCCESS_CALLBACK] = m_successHandlerCb;
        context->callback[FAIL_CALLBACK] = m_failHandlerCb;
        context->callback[COMPLETE_CALLBACK] = m_completeHandlerCb;
    } else {
        context->callback[SUCCESS_CALLBACK] = m_handlerCb;
        context->deferred = m_deferred;
    }
    return true;
}

bool LocatorCallbackHost::SendErrorCode(const int& errorCode)
{
    std::shared_lock<std::shared_mutex> guard(m_mutex);
    if (!IsSystemGeoLocationApi() && !IsSingleLocationRequest()) {
        LBSLOGE(LOCATOR_CALLBACK, "this is Callback type,cant send error msg.");
        return false;
    }
    if (m_env == nullptr) {
        LBSLOGE(LOCATOR_CALLBACK, "m_env is nullptr.");
        return false;
    }
    uv_loop_s *loop = nullptr;
    NAPI_CALL_BASE(m_env, napi_get_uv_event_loop(m_env, &loop), false);
    if (loop == nullptr) {
        LBSLOGE(LOCATOR_CALLBACK, "loop == nullptr.");
        return false;
    }
    uv_work_t *work = new (std::nothrow) uv_work_t;
    if (work == nullptr) {
        LBSLOGE(LOCATOR_CALLBACK, "work == nullptr.");
        return false;
    }
    AsyncContext *context = new (std::nothrow) AsyncContext(m_env);
    if (context == nullptr) {
        LBSLOGE(LOCATOR_CALLBACK, "context == nullptr.");
        return false;
    }
    if (!InitContext(context)) {
        LBSLOGE(LOCATOR_CALLBACK, "InitContext fail");
    }
    context->errCode = errorCode;
    work->data = context;
    DoSendErrorCode(loop, work);
    return true;
}

void LocatorCallbackHost::OnLocationReport(const std::unique_ptr<Location>& location)
{
    std::shared_lock<std::shared_mutex> guard(m_mutex);
    uv_loop_s *loop = nullptr;
    if (m_env == nullptr) {
        LBSLOGE(LOCATOR_CALLBACK, "m_env is nullptr.");
        return;
    }
    NAPI_CALL_RETURN_VOID(m_env, napi_get_uv_event_loop(m_env, &loop));
    if (loop == nullptr) {
        LBSLOGE(LOCATOR_CALLBACK, "loop == nullptr.");
        return;
    }
    uv_work_t *work = new (std::nothrow) uv_work_t;
    if (work == nullptr) {
        LBSLOGE(LOCATOR_CALLBACK, "work == nullptr.");
        return;
    }
    auto context = new (std::nothrow) LocationAsyncContext(m_env);
    if (context == nullptr) {
        LBSLOGE(LOCATOR_CALLBACK, "context == nullptr.");
        return;
    }
    auto asyncContext = static_cast<AsyncContext*>(context);
    if (!InitContext(asyncContext)) {
        LBSLOGE(LOCATOR_CALLBACK, "InitContext fail");
    }
    context->loc = std::make_unique<Location>(*location);
    work->data = context;
    DoSendWork(loop, work);
}

void LocatorCallbackHost::OnLocatingStatusChange(const int status)
{
}

void LocatorCallbackHost::OnErrorReport(const int errorCode)
{
    SendErrorCode(errorCode);
}

void LocatorCallbackHost::DeleteAllCallbacks()
{
    DeleteHandler();
    DeleteCompleteHandler();
    DeleteFailHandler();
    DeleteSuccessHandler();
}

void LocatorCallbackHost::DeleteHandler()
{
    LBSLOGD(LOCATOR_CALLBACK, "before DeleteHandler");
    std::shared_lock<std::shared_mutex> guard(m_mutex);
    if (m_handlerCb && m_env) {
        NAPI_CALL_RETURN_VOID(m_env, napi_delete_reference(m_env, m_handlerCb));
        m_handlerCb = nullptr;
    }
}

void LocatorCallbackHost::DeleteSuccessHandler()
{
    LBSLOGD(LOCATOR_CALLBACK, "before DeleteSuccessHandler");
    std::shared_lock<std::shared_mutex> guard(m_mutex);
    if (m_successHandlerCb && m_env) {
        NAPI_CALL_RETURN_VOID(m_env, napi_delete_reference(m_env, m_successHandlerCb));
        m_successHandlerCb = nullptr;
    }
}

void LocatorCallbackHost::DeleteFailHandler()
{
    LBSLOGD(LOCATOR_CALLBACK, "before DeleteFailHandler");
    std::shared_lock<std::shared_mutex> guard(m_mutex);
    if (m_failHandlerCb && m_env) {
        NAPI_CALL_RETURN_VOID(m_env, napi_delete_reference(m_env, m_failHandlerCb));
        m_failHandlerCb = nullptr;
    }
}

void LocatorCallbackHost::DeleteCompleteHandler()
{
    LBSLOGD(LOCATOR_CALLBACK, "before DeleteCompleteHandler");
    std::shared_lock<std::shared_mutex> guard(m_mutex);
    if (m_completeHandlerCb && m_env) {
        NAPI_CALL_RETURN_VOID(m_env, napi_delete_reference(m_env, m_completeHandlerCb));
        m_completeHandlerCb = nullptr;
    }
}

bool LocatorCallbackHost::IsSystemGeoLocationApi()
{
    return (m_successHandlerCb != nullptr) ? true : false;
}

bool LocatorCallbackHost::IsSingleLocationRequest()
{
    return (m_fixNumber == 1);
}

void LocatorCallbackHost::CountDown()
{
    if (IsSingleLocationRequest() && m_latch != nullptr) {
        m_latch->CountDown();
    }
}

void LocatorCallbackHost::Wait(int time)
{
    if (IsSingleLocationRequest() && m_latch != nullptr) {
        m_latch->Wait(time);
    }
}

int LocatorCallbackHost::GetCount()
{
    if (IsSingleLocationRequest() && m_latch != nullptr) {
        return m_latch->GetCount();
    }
    return 0;
}

void LocatorCallbackHost::SetCount(int count)
{
    if (IsSingleLocationRequest() && m_latch != nullptr) {
        return m_latch->SetCount(count);
    }
}
} // namespace Location
} // namespace OHOS
