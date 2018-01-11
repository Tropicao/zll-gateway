#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <znp.h>
#include "mt_zdo.h"

/********************************
 *       Constant data          *
 *******************************/
#define SCAN_ALL_CHANNELS_VALUE             0x07FFF800
#define MT_ZDO_NWK_DISCOVERY_TIMEOUT_MS     50

#define DEVICE_ANNCE_CAPABILITIES           0x8E

static const uint32_t scan_param = SCAN_ALL_CHANNELS_VALUE;
static SyncActionCb sync_action_cb = NULL;
static void (*_zdo_tc_dev_ind_cb)(uint16_t addr, uint64_t ext_addr) = NULL;

/********************************
 *     MT ZDO callbacks         *
 *******************************/

static uint8_t mt_zdo_state_change_ind_cb(uint8_t zdoState)
{
    LOG_INF("New ZDO state : 0x%02X", zdoState);
    if(zdoState == 0x09 && sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t mt_zdo_nwk_discovery_srsp_cb(NwkDiscoveryCnfFormat_t *msg)
{
    LOG_INF("ZDO Nwk discovery SRSP status : %02X", msg->Status);
    if(sync_action_cb)
        sync_action_cb();

    return 0;
}

static uint8_t mt_zdo_beacon_notify_ind_cb(BeaconNotifyIndFormat_t *msg)
{
    uint8_t index = 0;
    if(!msg)
        LOG_WARN("Beacon notification received without data");
    for(index = 0; index < msg->BeaconCount; index ++)
    {
        LOG_INF("=== New visible device in range ===");
        LOG_INF("Addr : 0x%04X - PAN : 0x%04X - Channel : 0x%02X",
                msg->BeaconList[index].SrcAddr, msg->BeaconList[index].PanId, msg->BeaconList[index].LogicalChannel);
        LOG_INF("===================================");
    }
    return 0;
}

static uint8_t mt_zdo_tc_dev_ind_cb(TcDevIndFormat_t *msg)
{
    if(!msg)
        LOG_WARN("Device indication received without data");
    LOG_INF("============== Device indication ============");
    LOG_INF("Addr : 0x%04X - ExtAddr : 0x%016X - Parent : 0x%04X",
            msg->SrcNwkAddr, msg->ExtAddr, msg->ParentNwkAddr);
    LOG_INF("=============================================");
    if(_zdo_tc_dev_ind_cb)
        _zdo_tc_dev_ind_cb(msg->SrcNwkAddr, msg->ExtAddr);
    return 0;
}

static uint8_t mt_zdo_device_annce_srsp_cb(DeviceAnnceSrspFormat_t *msg)
{
    if(!msg)
        LOG_WARN("ZDO Device Annce SRSP status : %02X", msg->Status);
    else
        LOG_INF("ZDO Device Annce SRSP status : %02X", msg->Status);

    if(sync_action_cb)
        sync_action_cb();

    return 0;

}

static mtZdoCb_t mt_zdo_cb = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mt_zdo_state_change_ind_cb,
    NULL,
    NULL,
    mt_zdo_beacon_notify_ind_cb,
    NULL,
    mt_zdo_nwk_discovery_srsp_cb,
    NULL,
    NULL,
    mt_zdo_tc_dev_ind_cb,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mt_zdo_device_annce_srsp_cb,
};

/********************************
 *          API                 *
 *******************************/

void mt_zdo_register_callbacks(void)
{
    zdoRegisterCallbacks(mt_zdo_cb);
}

void mt_zdo_nwk_discovery_req(SyncActionCb cb)
{
    NwkDiscoveryReqFormat_t req;
    uint8_t status;

    LOG_INF("Sending ZDO network discover request");
    if(cb)
        sync_action_cb = cb;
    memcpy(req.ScanChannels, &scan_param, 4);
    req.ScanDuration = 5;
    status = zdoNwkDiscoveryReq(&req);
    if (status != 0)
        LOG_ERR("Cannot start ZDO network discovery");
}

void mt_zdo_startup_from_app(SyncActionCb cb)
{
    LOG_INF("Starting ZDO stack");
    if(cb)
        sync_action_cb = cb;
    StartupFromAppFormat_t req;
    req.StartDelay = 0;
    zdoStartupFromApp(&req);
}

void mt_zdo_register_visible_device_cb(void (*cb)(uint16_t addr, uint64_t ext_addr))
{
    _zdo_tc_dev_ind_cb = cb;
}

void mt_zdo_device_annce(uint16_t addr, uint64_t uid, SyncActionCb cb)
{
    LOG_INF("Announce gateway (0x%04X - 0x%016X) to network", addr, uid);

    if(cb)
        sync_action_cb = cb;
    DeviceAnnceFormat_t req;
    req.NWKAddr = addr;
    memcpy(req.IEEEAddr, &uid, sizeof(uid));
    req.Capabilities = DEVICE_ANNCE_CAPABILITIES;
    zdoDeviceAnnce(&req);
}
