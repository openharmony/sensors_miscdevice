/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef VIBRATOR_SERVICE_CLIENT_H
#define VIBRATOR_SERVICE_CLIENT_H

#include <dlfcn.h>
#include <time.h>
#include <mutex>
#include <map>
#include <set>
#include <unordered_set>
#include <vector>

#include "iremote_object.h"
#include "singleton.h"

#include "i_vibrator_decoder.h"
#include "miscdevice_service_proxy.h"
#include "vibrator_agent_type.h"
#include "vibrator_client_stub.h"
#include "miscdevice_common.h"

namespace OHOS {
namespace Sensors {

struct VibratorDecodeHandle {
    void *handle;
    IVibratorDecoder *decoder;
    IVibratorDecoder *(*create)(const JsonParser &);
    void (*destroy)(IVibratorDecoder *);

    VibratorDecodeHandle(): handle(nullptr), decoder(nullptr),
        create(nullptr), destroy(nullptr) {}

    void Free()
    {
        if (handle != nullptr) {
            dlclose(handle);
            handle = nullptr;
        }
        decoder = nullptr;
        create = nullptr;
        destroy = nullptr;
    }
};

struct VibratorCurveInterval {
    int32_t beginTime;
    int32_t endTime;
    int32_t intensity;
    int32_t frequency;
};

class VibratorServiceClient : public Singleton<VibratorServiceClient> {
public:
    ~VibratorServiceClient() override;
    int32_t Vibrate(const VibratorIdentifier &identifier, int32_t timeOut, int32_t usage, bool systemUsage);
    int32_t Vibrate(const VibratorIdentifier &identifier, const std::string &effect,
        int32_t loopCount, int32_t usage, bool systemUsage);
#ifdef OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    int32_t PlayVibratorCustom(const VibratorIdentifier &identifier, const RawFileDescriptor &rawFd,
        int32_t usage, bool systemUsage, const VibratorParameter &parameter);
#endif // OHOS_BUILD_ENABLE_VIBRATOR_CUSTOM
    int32_t StopVibrator(const VibratorIdentifier &identifier, const std::string &mode);
    int32_t StopVibrator(const VibratorIdentifier &identifier);
    bool IsHdHapticSupported(const VibratorIdentifier &identifier);
    int32_t IsSupportEffect(const VibratorIdentifier &identifier, const std::string &effect, bool &state);
    void ProcessDeathObserver(const wptr<IRemoteObject> &object);
    int32_t PreProcess(const VibratorFileDescription &fd, VibratorPackage &package);
    int32_t GetDelayTime(const VibratorIdentifier &identifier, int32_t &delayTime);
    int32_t InitPlayPattern(const VibratorIdentifier &identifier, const VibratorPattern &pattern, int32_t usage,
        bool systemUsage, const VibratorParameter &parameter);
    int32_t PlayPattern(const VibratorIdentifier &identifier, const VibratorPattern &pattern, int32_t usage,
        bool systemUsage, const VibratorParameter &parameter);
    int32_t ConvertVibratorPackage(const VibratorPackage& inPkg, VibratePackage &outPkg);
    int32_t PlayPackageBySessionId(const VibratorIdentifier &identifier,
        const VibratorEffectParameter &vibratorEffectParameter, const VibratorPackage &package);
    int32_t StopVibrateBySessionId(const VibratorIdentifier &identifier, uint32_t sessionId);
    static int32_t FreeVibratorPackage(VibratorPackage &package);
    int32_t PlayPrimitiveEffect(const VibratorIdentifier &identifier, const std::string &effect,
        const PrimitiveEffect &primitiveEffect);
    bool IsSupportVibratorCustom(const VibratorIdentifier &identifier);
    int32_t SeekTimeOnPackage(int32_t seekTime, const VibratorPackage &completePackage, VibratorPackage &seekPackage);
    int32_t SubscribeVibratorPlugInfo(const VibratorUser *user);
    int32_t UnsubscribeVibratorPlugInfo(const VibratorUser *user);
    std::set<RecordVibratorPlugCallback> GetSubscribeUserCallback(int32_t deviceId);
    bool HandleVibratorData(VibratorStatusEvent statusEvent);
    void SetUsage(const VibratorIdentifier &identifier, int32_t usage, bool systemUsage);
    void SetLoopCount(const VibratorIdentifier &identifier, int32_t count);
    void SetParameters(const VibratorIdentifier &identifier, const VibratorParameter &parameter);
    VibratorEffectParameter GetVibratorEffectParameter(const VibratorIdentifier &identifier);
    int32_t GetVibratorList(const VibratorIdentifier& identifier, std::vector<VibratorInfos>& vibratorInfo);
    int32_t GetEffectInfo(const VibratorIdentifier& identifier, const std::string& effectType, EffectInfo& effectInfo);
    static int32_t ModulatePackage(const VibratorEvent &modulationCurve,
        const VibratorPackage &beforeModulationPackage, VibratorPackage &afterModulationPackage);
    int32_t DisableVibratorByPid(int32_t pid);
    int32_t EnableVibratorByPid(int32_t pid);

private:
    int32_t InitServiceClient();
    int32_t LoadDecoderLibrary(const std::string& path);
    int32_t ConvertVibratorPackage(const VibratePackage& inPkg, VibratorPackage &outPkg);
    int32_t TransferClientRemoteObject();
    int32_t GetVibratorCapacity(const VibratorIdentifier &identifier, VibratorCapacity &capacity);
    void ConvertSeekVibratorPackage(const VibratorPackage &completePackage, VibratePackage &convertPackage,
        int32_t seekTime);
    void ConvertVibratorPattern(const VibratorPattern &vibratorPattern, VibratePattern &vibratePattern);
    bool SkipEventAndConvertVibratorEvent(const VibratorEvent &vibratorEvent, VibratePattern &vibratePattern,
        int32_t patternStartTime, VibrateEvent &vibrateEvent);
    static VibratorCurvePoint* GetCurveListAfterModulation(const std::vector<VibratorCurveInterval>& modInterval,
        const VibratorEvent &beforeModEvent, int32_t patternOffset);
    static int32_t ModulateEventWithoutCurvePoints(const std::vector<VibratorCurveInterval>& modInterval,
        int32_t patternStartTime, const VibratorEvent &beforeModEvent, VibratorEvent &afterModEvent);
    static void FreePartiallyAllocatedVibratorPatterns(VibratorPattern** patterns, int32_t partialIdx);
    static int32_t ModulateVibratorPattern(const VibratorEvent &modulationCurve,
        const VibratorPattern &beforeModulationPattern, VibratorPattern &afterModulationPattern);
    static void FreePartiallyAllocatedVibratorEvents(VibratorEvent** events, int32_t partialIdx);
    static int32_t ModulateVibratorEvent(const VibratorEvent &modulationCurve, int32_t patternStartTime,
        const VibratorEvent &beforeModulationEvent, VibratorEvent &afterModulationEvent);
    static int32_t ModulateEventWithCurvePoints(std::vector<VibratorCurveInterval>& modInterval,
        int32_t patternStartTime, const VibratorEvent &beforeModulationEvent, VibratorEvent &afterModulationEvent);
    static void BinarySearchInterval(const std::vector<VibratorCurveInterval>& interval,
        int32_t val, int32_t& idx);
    static void ConvertVibratorEventsToCurveIntervals(const VibratorEvent &vibratorEvent, int32_t patternTimeOffset,
        std::vector<VibratorCurveInterval>& curveInterval);
    static int32_t RestrictIntensityRange(int32_t intensity);
    static int32_t RestrictFrequencyRange(int32_t frequency);
    void WriteVibratorHiSysIPCEvent(IMiscdeviceServiceIpcCode code, int32_t ret);
    void WriteOtherHiSysIPCEvent(IMiscdeviceServiceIpcCode code, int32_t ret);
    sptr<IRemoteObject::DeathRecipient> serviceDeathObserver_ = nullptr;
    sptr<IMiscdeviceService> miscdeviceProxy_ = nullptr;
    sptr<VibratorClientStub> vibratorClient_ = nullptr;
    VibratorDecodeHandle decodeHandle_;
    std::mutex clientMutex_;
    std::mutex decodeMutex_;

    std::recursive_mutex subscribeMutex_;
    std::set<const VibratorUser *> subscribeSet_;

    std::mutex vibratorEffectMutex_;
    std::map<VibratorIdentifier, VibratorEffectParameter> vibratorEffectMap_;
    std::unordered_set<std::string> supportedEffectSet_;
};
} // namespace Sensors
} // namespace OHOS
#endif // VIBRATOR_SERVICE_CLIENT_H
