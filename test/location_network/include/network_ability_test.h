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

#ifdef FEATURE_NETWORK_SUPPORT
#ifndef NETWORK_ABILITY_TEST_H
#define NETWORK_ABILITY_TEST_H

#include <gtest/gtest.h>

#include "network_ability.h"
#include "network_ability_proxy.h"

namespace OHOS {
namespace Location {
class NetworkAbilityTest : public testing::Test {
public:
    void SetUp();
    void TearDown();
    void MockNativePermission();
    
    sptr<NetworkAbilityProxy> proxy_;
    sptr<NetworkAbility> ability_;
};
} // namespace Location
} // namespace OHOS
#endif // NETWORK_ABILITY_TEST_H
#endif // FEATURE_NETWORK_SUPPORT